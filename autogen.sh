#!/bin/sh

test -f configure.ac || {
  echo "Please, run this script in the top level project directory."
  exit
}

autoconf
autoheader
