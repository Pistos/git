static int use_size_cache;

			printf("  Bin\n");
static int mmfile_is_binary(mmfile_t *mf)
	long sz = mf->size;
	return !!memchr(mf->ptr, 0, sz);
	if (!o->text && (mmfile_is_binary(&mf1) || mmfile_is_binary(&mf2))) {
		return;
	if (mmfile_is_binary(&mf1) || mmfile_is_binary(&mf2))
	else {
	if (mmfile_is_binary(&mf2))
		return;
static struct sha1_size_cache {
	unsigned char sha1[20];
	unsigned long size;
} **sha1_size_cache;
static int sha1_size_cache_nr, sha1_size_cache_alloc;

static struct sha1_size_cache *locate_size_cache(unsigned char *sha1,
						 int find_only,
						 unsigned long size)
{
	int first, last;
	struct sha1_size_cache *e;

	first = 0;
	last = sha1_size_cache_nr;
	while (last > first) {
		int cmp, next = (last + first) >> 1;
		e = sha1_size_cache[next];
		cmp = hashcmp(e->sha1, sha1);
		if (!cmp)
			return e;
		if (cmp < 0) {
			last = next;
			continue;
		}
		first = next+1;
	}
	/* not found */
	if (find_only)
		return NULL;
	/* insert to make it at "first" */
	if (sha1_size_cache_alloc <= sha1_size_cache_nr) {
		sha1_size_cache_alloc = alloc_nr(sha1_size_cache_alloc);
		sha1_size_cache = xrealloc(sha1_size_cache,
					   sha1_size_cache_alloc *
					   sizeof(*sha1_size_cache));
	}
	sha1_size_cache_nr++;
	if (first < sha1_size_cache_nr)
		memmove(sha1_size_cache + first + 1, sha1_size_cache + first,
			(sha1_size_cache_nr - first - 1) *
			sizeof(*sha1_size_cache));
	e = xmalloc(sizeof(struct sha1_size_cache));
	sha1_size_cache[first] = e;
	hashcpy(e->sha1, sha1);
	e->size = size;
	return e;
}

	if (!use_size_cache)
		size_only = 0;
	if (s->data)
		return err;
		buf = s->data;
		if (convert_to_git(s->path, &buf, &size)) {
		struct sha1_size_cache *e;

		if (size_only && use_size_cache &&
		    (e = locate_size_cache(s->sha1, 1, 0)) != NULL) {
			s->size = e->size;
			return 0;
		}

		if (size_only) {
			if (use_size_cache && 0 < type)
				locate_size_cache(s->sha1, 0, s->size);
		}
	s->should_free = s->should_munmap = 0;
	s->data = NULL;
			if ((!fill_mmfile(&mf, one) && mmfile_is_binary(&mf)) ||
			    (!fill_mmfile(&mf, two) && mmfile_is_binary(&mf)))
	if (options->setup & DIFF_SETUP_USE_SIZE_CACHE)
		use_size_cache = 1;
		if (mmfile_is_binary(&mf2))