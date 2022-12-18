#!/bin/sh
set -e
git_root_dir=`git rev-parse --show-toplevel`
cd $git_root_dir
rg --files | rg '^.*\.(c|h|cc|cxx|cpp|hpp)$' | xargs clang-format -i
