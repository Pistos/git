			printf("  Bin\n");
static int mmfile_is_binary(mmfile_t *mf)
	long sz = mf->size;
	return !!memchr(mf->ptr, 0, sz);
	if (!o->text && (mmfile_is_binary(&mf1) || mmfile_is_binary(&mf2))) {
	if (mmfile_is_binary(&mf1) || mmfile_is_binary(&mf2))
	else {
	if (mmfile_is_binary(&mf2))
		buf = s->data;
		if (convert_to_git(s->path, &buf, &size)) {
			if ((!fill_mmfile(&mf, one) && mmfile_is_binary(&mf)) ||
			    (!fill_mmfile(&mf, two) && mmfile_is_binary(&mf)))
		if (mmfile_is_binary(&mf2))