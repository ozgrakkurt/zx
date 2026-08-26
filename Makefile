cfmt:
	clang-format -i -style=file src/*/lib/*.cc src/*/mod/*.cppm src/*/test/*.cc src/*/bin/*.cc
git_repack:
	git repack -a -d -f --depth=250 --window=250
