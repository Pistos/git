#include "cache.h"
#include "fetch.h"
#include "commit.h"
#include "tree.h"
#include "tree-walk.h"
#include "tag.h"
#include "blob.h"
#include "refs.h"
#include "strbuf.h"

int get_tree = 0;
int get_history = 0;
int get_all = 0;
int get_verbosely = 0;
int get_recover = 0;
static unsigned char current_commit_sha1[20];

void pull_say(const char *fmt, const char *hex)
{
	if (get_verbosely)
		fprintf(stderr, fmt, hex);
}

static void report_missing(const struct object *obj)
{
	char missing_hex[41];
	strcpy(missing_hex, sha1_to_hex(obj->sha1));;
	fprintf(stderr, "Cannot obtain needed %s %s\n",
		obj->type ? typename(obj->type): "object", missing_hex);
	if (!is_null_sha1(current_commit_sha1))
		fprintf(stderr, "while processing commit %s.\n",
			sha1_to_hex(current_commit_sha1));
}

static int process(struct object *obj);

static int process_tree(struct tree *tree)
{
	struct tree_desc desc;
	struct name_entry entry;

	if (parse_tree(tree))
		return -1;

	init_tree_desc(&desc, tree->buffer, tree->size);
	while (tree_entry(&desc, &entry)) {
		struct object *obj = NULL;

		if (S_ISDIR(entry.mode)) {
			struct tree *tree = lookup_tree(entry.sha1);
			if (tree)
				obj = &tree->object;
		}
		else {
			struct blob *blob = lookup_blob(entry.sha1);
			if (blob)
				obj = &blob->object;
		}
		if (!obj || process(obj))
			return -1;
	}
	free(tree->buffer);
	tree->buffer = NULL;
	tree->size = 0;
	return 0;
}

#define COMPLETE	(1U << 0)
#define SEEN		(1U << 1)
#define TO_SCAN		(1U << 2)

static struct commit_list *complete = NULL;

static int process_commit(struct commit *commit)
{
	if (parse_commit(commit))
		return -1;

	while (complete && complete->item->date >= commit->date) {
		pop_most_recent_commit(&complete, COMPLETE);
	}

	if (commit->object.flags & COMPLETE)
		return 0;

	hashcpy(current_commit_sha1, commit->object.sha1);

	pull_say("walk %s\n", sha1_to_hex(commit->object.sha1));

	if (get_tree) {
		if (process(&commit->tree->object))
			return -1;
		if (!get_all)
			get_tree = 0;
	}
	if (get_history) {
		struct commit_list *parents = commit->parents;
		for (; parents; parents = parents->next) {
			if (process(&parents->item->object))
				return -1;
		}
	}
	return 0;
}

static int process_tag(struct tag *tag)
{
	if (parse_tag(tag))
		return -1;
	return process(tag->tagged);
}

static struct object_list *process_queue = NULL;
static struct object_list **process_queue_end = &process_queue;

static int process_object(struct object *obj)
{
	if (obj->type == OBJ_COMMIT) {
		if (process_commit((struct commit *)obj))
			return -1;
		return 0;
	}
	if (obj->type == OBJ_TREE) {
		if (process_tree((struct tree *)obj))
			return -1;
		return 0;
	}
	if (obj->type == OBJ_BLOB) {
		return 0;
	}
	if (obj->type == OBJ_TAG) {
		if (process_tag((struct tag *)obj))
			return -1;
		return 0;
	}
	return error("Unable to determine requirements "
		     "of type %s for %s",
		     typename(obj->type), sha1_to_hex(obj->sha1));
}

static int process(struct object *obj)
{
	if (obj->flags & SEEN)
		return 0;
	obj->flags |= SEEN;

	if (has_sha1_file(obj->sha1)) {
		/* We already have it, so we should scan it now. */
		obj->flags |= TO_SCAN;
	}
	else {
		if (obj->flags & COMPLETE)
			return 0;
		prefetch(obj->sha1);
	}

	object_list_insert(obj, process_queue_end);
	process_queue_end = &(*process_queue_end)->next;
	return 0;
}

static int loop(void)
{
	struct object_list *elem;

	while (process_queue) {
		struct object *obj = process_queue->item;
		elem = process_queue;
		process_queue = elem->next;
		free(elem);
		if (!process_queue)
			process_queue_end = &process_queue;

		/* If we are not scanning this object, we placed it in
		 * the queue because we needed to fetch it first.
		 */
		if (! (obj->flags & TO_SCAN)) {
			if (fetch(obj->sha1)) {
				report_missing(obj);
				return -1;
			}
		}
		if (!obj->type)
			parse_object(obj->sha1);
		if (process_object(obj))
			return -1;
	}
	return 0;
}

static int interpret_target(char *target, unsigned char *sha1)
{
	if (!get_sha1_hex(target, sha1))
		return 0;
	if (!check_ref_format(target)) {
		if (!fetch_ref(target, sha1)) {
			return 0;
		}
	}
	return -1;
}

static int mark_complete(const char *path, const unsigned char *sha1, int flag, void *cb_data)
{
	struct commit *commit = lookup_commit_reference_gently(sha1, 1);
	if (commit) {
		commit->object.flags |= COMPLETE;
		insert_by_date(commit, &complete);
	}
	return 0;
}

int pull_targets_stdin(char ***target, const char ***write_ref)
{
	int targets = 0, targets_alloc = 0;
	struct strbuf buf;
	*target = NULL; *write_ref = NULL;
	strbuf_init(&buf);
	while (1) {
		char *rf_one = NULL;
		char *tg_one;

		read_line(&buf, stdin, '\n');
		if (buf.eof)
			break;
		tg_one = buf.buf;
		rf_one = strchr(tg_one, '\t');
		if (rf_one)
			*rf_one++ = 0;

		if (targets >= targets_alloc) {
			targets_alloc = targets_alloc ? targets_alloc * 2 : 64;
			*target = xrealloc(*target, targets_alloc * sizeof(**target));
			*write_ref = xrealloc(*write_ref, targets_alloc * sizeof(**write_ref));
		}
		(*target)[targets] = xstrdup(tg_one);
		(*write_ref)[targets] = rf_one ? xstrdup(rf_one) : NULL;
		targets++;
	}
	return targets;
}

void pull_targets_free(int targets, char **target, const char **write_ref)
{
	while (targets--) {
		free(target[targets]);
		if (write_ref && write_ref[targets])
			free((char *) write_ref[targets]);
	}
}

int pull(int targets, char **target, const char **write_ref,
         const char *write_ref_log_details)
{
	struct ref_lock **lock = xcalloc(targets, sizeof(struct ref_lock *));
	unsigned char *sha1 = xmalloc(targets * 20);
	char *msg;
	int ret;
	int i;

	save_commit_buffer = 0;
	track_object_refs = 0;

	for (i = 0; i < targets; i++) {
		if (!write_ref || !write_ref[i])
			continue;

		lock[i] = lock_ref_sha1(write_ref[i], NULL);
		if (!lock[i]) {
			error("Can't lock ref %s", write_ref[i]);
			goto unlock_and_fail;
		}
	}

	if (!get_recover)
		for_each_ref(mark_complete, NULL);

	for (i = 0; i < targets; i++) {
		if (interpret_target(target[i], &sha1[20 * i])) {
			error("Could not interpret %s as something to pull", target[i]);
			goto unlock_and_fail;
		}
		if (process(lookup_unknown_object(&sha1[20 * i])))
			goto unlock_and_fail;
	}

	if (loop())
		goto unlock_and_fail;

	if (write_ref_log_details) {
		msg = xmalloc(strlen(write_ref_log_details) + 12);
		sprintf(msg, "fetch from %s", write_ref_log_details);
	} else {
		msg = NULL;
	}
	for (i = 0; i < targets; i++) {
		if (!write_ref || !write_ref[i])
			continue;
		ret = write_ref_sha1(lock[i], &sha1[20 * i], msg ? msg : "fetch (unknown)");
		lock[i] = NULL;
		if (ret)
			goto unlock_and_fail;
	}
	free(msg);

	return 0;


unlock_and_fail:
	for (i = 0; i < targets; i++)
		if (lock[i])
			unlock_ref(lock[i]);
	return -1;
}