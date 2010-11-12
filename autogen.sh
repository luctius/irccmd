#!/bin/sh

aclocal
autoheader
git log > ChangeLog
automake --add-missing --copy
autoconf
