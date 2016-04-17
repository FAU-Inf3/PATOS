#!/bin/bash

# start clang to get default search paths
# the output look like this:
#
#   ...
#   #include <...> search starts here:
#      <path 1>
#      ...
#      <path n>
#   Enf of search list.
#   ...
#
# use awk to extract search paths
search_paths=$(clang-3.5 -x cl -E -v /dev/null 2>&1 | sed -e 's/^[[:space:]]*//' | awk 'BEGIN { inside = 0 ; list = "" } /End of search list./ { inside = 0 } inside { list = list "-I " $0 " " } /#include <...> search starts here:/ { inside = 1 } END { print list }')

bin/patos "$@" $search_paths
