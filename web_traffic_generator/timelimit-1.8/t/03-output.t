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

echo '1..9'

echo '# kill a shell script during the sleep, only execute a single echo'
v=`$TIMELIMIT -t 1 sh -c 'echo 1; sleep 3; echo 2' 2>&1 > /dev/null`
res="$?"
if [ "$res" = 143 ]; then echo 'ok 1'; else echo "not ok 1 exit code $res"; fi
if expr "x$v" : 'x.*warning' > /dev/null; then echo 'ok 2'; else echo "not ok 2 v is '$v'"; fi
if ! expr "x$v" : 'x.*kill' > /dev/null; then echo 'ok 3'; else echo "not ok 3 v is '$v'"; fi

echo '# catch the warning signal, run two out of three echos'
v=`$TIMELIMIT -t 1 -T 4 sh -c 'trap "" TERM; echo 1; sleep 3; echo 2; sleep 3; echo 3' 2>&1 > /dev/null`
res="$?"
if [ "$res" = 137 ]; then echo 'ok 4'; else echo "not ok 4 exit code $res"; fi
if expr "x$v" : 'x.*warning' > /dev/null; then echo 'ok 5'; else echo "not ok 5 v is '$v'"; fi
if expr "x$v" : 'x.*kill' > /dev/null; then echo 'ok 6'; else echo "not ok 6 v is '$v'"; fi

echo '# now do the same, but be vewwy, vewwy quiet...'
v=`$TIMELIMIT -q -t 1 -T 4 sh -c 'trap "" TERM; echo 1; sleep 3; echo 2; sleep 3; echo 3' 2>&1 > /dev/null`
res="$?"
if [ "$res" = 137 ]; then echo 'ok 7'; else echo "not ok 7 exit code $res"; fi
if ! expr "x$v" : 'x.*warning' > /dev/null; then echo 'ok 8'; else echo "not ok 8 v is '$v'"; fi
if ! expr "x$v" : 'x.*kill' > /dev/null; then echo 'ok 9'; else echo "not ok 9 v is '$v'"; fi
