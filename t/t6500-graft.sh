#!/bin/sh

test_description='graft and ancestry traversal'
. test-lib.sh

# Real history:
#
#      .----D
#     /
#    A--B--C--E
#
# Grafted history:
#
#      .----D
#     /      \
#    A-----C--E
#

advance_one () {
	echo "$1" >file &&
	git add file &&
	test_tick &&
	git commit -m "$1" &&
	git rev-parse --verify HEAD >"$1"
}

test_expect_success setup '
	advance_one A &&
	advance_one D &&

	git reset --hard $(cat A) &&
	advance_one B &&
	advance_one C &&
	advance_one E &&

	(
		echo $(cat E) $(cat C) $(cat D)
		echo $(cat C) $(cat A)
	) >.git/info/grafts &&

	git log --graph --pretty=oneline --abbrev-commit --parents &&
	echo " - - - - - - - - - - " &&
	git log --graph --pretty=oneline --abbrev-commit --parents --ignore-graft

'

test_expect_success 'clone should lose grafts' '
	git clone --bare "file://$(pwd)/.git" cloned.git &&
	(
		GIT_DIR=cloned.git && export GIT_DIR &&
		git log --graph --pretty=oneline --abbrev-commit --parents &&
		git cat-file commit $(cat B) &&
		test_must_fail git cat-file commit $(cat D) &&
		git fsck --full
	)
'

test_expect_success 'push should lose grafts' '
	test_create_repo pushed.git &&
	(
		cd pushed.git &&
		git fetch ../.git master:refs/heads/master &&
		git log --graph --pretty=oneline --abbrev-commit --parents &&
		git cat-file commit $(cat ../B) &&
		test_must_fail git cat-file commit $(cat ../D) &&
		git fsck --full
	)
'

test_done