#!/usr/bin/env bash
#
# Test harness for generating files under tests/rust_protobuf/v[23]
# and expecting them either to succeed or fail for some known reason.
#

# Checked in the end for non-empty value which serves as a boolean flag
have_failures=""

# Expected codegen failures are marked in the associative array `must_fail`
# with the relative path as the key and reason as value.
# When adding new, remember not to add any whitespace around `=`.
declare -A must_fail

must_fail["v2/test_group_pb.proto"]="expected failure (empty read)"
must_fail["v3/test_enum_alias_pb.proto"]="enum alias not implemented"
must_fail["v2/test_enum_alias_pb.proto"]="enum alias not implemented"
must_fail["v2/test_expose_oneof_pb.proto"]="missing file"
must_fail["v2/test_enum_invalid_default.proto"]="enum variant does not exist"
must_fail["v3/test_enum_invalid_default.proto"]="enum variant does not exist"

# Custom arguments to pass to `pb-rs` for generating files used in testing
declare -A custom_pbrs_args

custom_pbrs_args["v2/test_owned_pb.proto"]="--owned"
custom_pbrs_args["v3/test_owned_pb.proto"]="--owned"
custom_pbrs_args["v2/test_deprecated_lifetime_can_compile.proto"]="--add-deprecated-fields"
custom_pbrs_args["v3/test_deprecated_lifetime_can_compile.proto"]="--add-deprecated-fields"

# Combined stdout and stderr for codegen of unexpectedly failed file.
declare -A outs

expecting_failure() {
	[ "${must_fail["$1"]+_}" ]
}

expecting_success() {
	! expecting_failure "$1"
}

success_msg() {
	if expecting_failure "$1"; then
		echo "${must_fail["$1"]}"
	else
		echo "ok"
	fi
}

for f in v[23]/*.proto; do
	ret=0
	rm -f "${f%.proto}.rs"
	out="$(cargo run -p pb-rs --quiet -- ${custom_pbrs_args[$f]} "$f" 2>&1)" || ret=$?

	if expecting_failure "$f" && [ "$ret" -eq 0 ]; then
		outs["$f"]="$out"
		have_failures="true"
		echo "$f: unexpected success"
	elif expecting_success "$f" && [ "$ret" -ne 0 ]; then
		have_failures="true"
		outs["$f"]="$out"
		echo "$f: unexpected failure $ret"
	else
		echo "$f: $(success_msg "$f")"
	fi
done

for f in common/*.proto; do
	ret=0
	rm -f "${f%.proto}.rs"
	out="$(cargo run -p pb-rs --quiet -- ${custom_pbrs_args[$f]} "$f" 2>&1)" || ret=$?

	if expecting_failure "$f" && [ "$ret" -eq 0 ]; then
		outs["$f"]="$out"
		have_failures="true"
		echo "$f: unexpected success"
	elif expecting_success "$f" && [ "$ret" -ne 0 ]; then
		have_failures="true"
		outs["$f"]="$out"
		echo "$f: unexpected failure $ret"
	else
		echo "$f: $(success_msg "$f")"
	fi
done

echo

if [ "$have_failures" ]; then
	echo "There were code generation failures:"
	for f in "${!outs[@]}"; do
		echo
		echo "$f:"
		echo "${outs["$f"]}"
	done
	exit 1
else
	echo "All files generated as expected"
fi
