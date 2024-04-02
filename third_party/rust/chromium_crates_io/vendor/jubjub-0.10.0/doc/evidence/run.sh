#!/bin/sh
sage verify.sage .
grep -Rn '.' verify-* |grep '^verify-.*:1:' |sed 's/:1:/ = /'

