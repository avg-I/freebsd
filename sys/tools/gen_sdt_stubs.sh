#!/bin/sh
#
# Copyright (C) 2014 The FreeBSD Project. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# $FreeBSD$ 
#
# $1: filename of relocatable kernel object (before final linking)
#

cat <<-EOF

	.pushsection sdt_stub_functions

EOF

nm -u $1 | fgrep __dtrace_sdt_call_ | sort | uniq | while read type func ; do
	cat <<-EOF
		.global $func
		.type $func, @function
		$func:

	EOF
done

cat <<-EOF

	.popsection

	.pushsection sdt_call_placess, "a"

EOF

objdump -r -j .text $1 | fgrep __dtrace_sdt_call_ | while read off reltype relval ; do
	func=${relval%+*}
	cat <<-EOF
		.align 8
		1:
		.quad text + 0x${off}
		.asciz "${func}"

		.pushsection set_sdt_calls, "a"
		.quad 1b
		.popsection

	EOF
done
cat <<-EOF
	.popsection

	.global __start_set_sdt_calls
	.global __stop_set_sdt_calls
EOF
