#!/bin/sh
#
# Copyright (c) 2011  Peter Pentchev
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

[ -z "$TIMELIMIT" ] && TIMELIMIT='./timelimit'

echo '1..35'

echo '# timelimit with no arguments should exit with EX_USAGE'
$TIMELIMIT > /dev/null 2>&1
res="$?"
if [ "$res" = 64 ]; then echo 'ok 1'; else echo "not ok 1 exit code $res"; fi

echo '# kill a shell script during the sleep, only execute a single echo'
v=`$TIMELIMIT -t 1 sh -c 'echo 1; sleep 3; echo 2' 2>/dev/null`
res="$?"
if [ "$res" = 143 ]; then echo 'ok 2'; else echo "not ok 2 exit code $res"; fi
if expr "x$v" : 'x1' > /dev/null; then echo 'ok 3'; else echo "not ok 3 v is '$v'"; fi
if ! expr "x$v" : 'x.*2' > /dev/null; then echo 'ok 4'; else echo "not ok 4 v is '$v'"; fi

echo '# give the shell script time enough to execute both echos'
v=`$TIMELIMIT -t 4 sh -c 'echo 1; sleep 3; echo 2' 2>/dev/null`
res="$?"
if [ "$res" = 0 ]; then echo 'ok 5'; else echo "not ok 5 exit code $res"; fi
if expr "x$v" : 'x1' > /dev/null; then echo 'ok 6'; else echo "not ok 6 v is '$v'"; fi
if expr "x$v" : 'x.*2' > /dev/null; then echo 'ok 7'; else echo "not ok 7 v is '$v'"; fi

echo '# catch the warning signal, run two out of three echos'
v=`$TIMELIMIT -t 1 -T 4 sh -c 'trap "" TERM; echo 1; sleep 3; echo 2; sleep 3; echo 3' 2>/dev/null`
res="$?"
if [ "$res" = 137 ]; then echo 'ok 8'; else echo "not ok 8 exit code $res"; fi
if expr "x$v" : 'x1' > /dev/null; then echo 'ok 9'; else echo "not ok 9 v is '$v'"; fi
if expr "x$v" : 'x.*2' > /dev/null; then echo 'ok 10'; else echo "not ok 10 v is '$v'"; fi
if ! expr "x$v" : 'x.*3' > /dev/null; then echo 'ok 11'; else echo "not ok 11 v is '$v'"; fi

echo '# catch the warning signal and still run to completion'
v=`$TIMELIMIT -t 1 -T 12 sh -c 'trap "" TERM; echo 1; sleep 3; echo 2; sleep 3; echo 3' 2>/dev/null`
res="$?"
if [ "$res" = 0 ]; then echo 'ok 12'; else echo "not ok 12 exit code $res"; fi
if expr "x$v" : 'x1' > /dev/null; then echo 'ok 13'; else echo "not ok 13 v is '$v'"; fi
if expr "x$v" : 'x.*2' > /dev/null; then echo 'ok 14'; else echo "not ok 14 v is '$v'"; fi
if expr "x$v" : 'x.*3' > /dev/null; then echo 'ok 15'; else echo "not ok 15 v is '$v'"; fi

echo '# now interrupt with a different signal'
v=`$TIMELIMIT -t 1 -s 1 sh -c 'echo 1; sleep 3; echo 2' 2>/dev/null`
res="$?"
if [ "$res" = 129 ]; then echo 'ok 16'; else echo "not ok 16 exit code $res"; fi
if expr "x$v" : 'x1' > /dev/null; then echo 'ok 17'; else echo "not ok 17 v is '$v'"; fi
if ! expr "x$v" : 'x.*2' > /dev/null; then echo 'ok 18'; else echo "not ok 18 v is '$v'"; fi

echo '# now catch that different signal'
v=`$TIMELIMIT -t 1 -s 1 sh -c 'trap "" HUP; echo 1; sleep 3; echo 2' 2>/dev/null`
res="$?"
if [ "$res" = 0 ]; then echo 'ok 19'; else echo "not ok 19 exit code $res"; fi
if expr "x$v" : 'x1' > /dev/null; then echo 'ok 20'; else echo "not ok 20 v is '$v'"; fi
if expr "x$v" : 'x.*2' > /dev/null; then echo 'ok 21'; else echo "not ok 21 v is '$v'"; fi

echo '# now kill with a yet different signal'
v=`$TIMELIMIT -t 1 -s 1 -T 1 -S 15 sh -c 'trap "" HUP; echo 1; sleep 3; echo 2' 2>/dev/null`
res="$?"
if [ "$res" = 143 ]; then echo 'ok 22'; else echo "not ok 22 exit code $res"; fi
if expr "x$v" : 'x1' > /dev/null; then echo 'ok 23'; else echo "not ok 23 v is '$v'"; fi
if ! expr "x$v" : 'x.*2' > /dev/null; then echo 'ok 24'; else echo "not ok 24 v is '$v'"; fi

echo '# timelimit with an invalid argument should exit with EX_USAGE'
$TIMELIMIT -X > /dev/null 2>&1
res="$?"
if [ "$res" = 64 ]; then echo 'ok 25'; else echo "not ok 25 exit code $res"; fi

echo '# use invalid numbers for the various options'
v=`$TIMELIMIT -t x true 2>/dev/null`
res="$?"
if [ "$res" = 64 ]; then echo 'ok 26'; else echo "not ok 26 -t x exit code $res"; fi
v=`$TIMELIMIT -T x true 2>/dev/null`
res="$?"
if [ "$res" = 64 ]; then echo 'ok 27'; else echo "not ok 27 -T x exit code $res"; fi
v=`$TIMELIMIT -s x true 2>/dev/null`
res="$?"
if [ "$res" = 64 ]; then echo 'ok 28'; else echo "not ok 28 -s x exit code $res"; fi
v=`$TIMELIMIT -S x true 2>/dev/null`
res="$?"
if [ "$res" = 64 ]; then echo 'ok 29'; else echo "not ok 29 -S x exit code $res"; fi
v=`$TIMELIMIT -s 1.5 true 2>/dev/null`
res="$?"
if [ "$res" = 64 ]; then echo 'ok 30'; else echo "not ok 30 -s 1.5 exit code $res"; fi
v=`$TIMELIMIT -S 1.5 true 2>/dev/null`
res="$?"
if [ "$res" = 64 ]; then echo 'ok 31'; else echo "not ok 31 -S 1.5 exit code $res"; fi
v=`$TIMELIMIT -t '' true 2>/dev/null`
res="$?"
if [ "$res" = 64 ]; then echo 'ok 32'; else echo "not ok 32 -t '' exit code $res"; fi
v=`$TIMELIMIT -T '' true 2>/dev/null`
res="$?"
if [ "$res" = 64 ]; then echo 'ok 33'; else echo "not ok 33 -T '' exit code $res"; fi
v=`$TIMELIMIT -s '' true 2>/dev/null`
res="$?"
if [ "$res" = 64 ]; then echo 'ok 34'; else echo "not ok 34 -s '' exit code $res"; fi
v=`$TIMELIMIT -S '' true 2>/dev/null`
res="$?"
if [ "$res" = 64 ]; then echo 'ok 35'; else echo "not ok 35 -S '' exit code $res"; fi
