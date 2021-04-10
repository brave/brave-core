#!/usr/bin/env bash
# Requirements: bash, coreutils, git, pylint.
# This is called via `npm run pylint' with the top of the brave-core repo as $PWD.

# Fail on any error.
shopt -s inherit_errexit
set -eEo pipefail

# Defaults. Overwritten in parse_cmdline() and set readonly.
base_branch=""
check_folders=(build components installer script tools)
pylint_options=(-j0 -rn --rcfile=.pylintrc)
report=0
report_file=pylint-report.txt

# Print a help message and exit with 1 by default.
# $@ := [ exit_status ]
help() {
	cat <<-EOF
	USAGE: pylint.sh --help
	       pylint.sh [--compare-with <base_branch>] [--report]

	Run pylint on versioned python scripts in:
	${check_folders[*]/%/\/}

	--compare-with <base_branch>: only analyse files changed relative to base_branch
	--report: produce a parseable report file
	
	The .pylintrc file at the top of this repo defines pylint options.
	
	This script should be called via \`npm run', e.g.:
	npm run pylint
	npm run pylint -- --compare-with master
	EOF
	exit "${1:-1}"
}

# Print an error message, followed by the help message and exit.
# $@ := message
error() { printf "%s\n\n" "ERROR: $*" >&2; help; }

# Parse command line arguments.
# $@ := script_arguments
parse_cmdline() {
    for ((i=1; i <= $#; ++i)); do
        case "${@:$i:1}" in
            --compare-with)
                ((++i <= $#)) || error "The --compare-with option requires an argument";
                base_branch="${*:$i:1}";;
            --report) report=1;;
            --help) help 0;;
            *) error "Invalid arguments passed: ${*:$i}";;
        esac
    done
    readonly base_branch check_folders pylint_options report report_file
}

# Print a tab-delimited list of python scripts changed relative to $base_branch.
# $@ := ""
get_changed_files() {
    git diff --name-only --diff-filter drt --merge-base "origin/${base_branch:?}" -- \
        "${check_folders[@]/%/\/\*.py}"|tr '\n' '\t'
}

# Execute pylint (if necessary) and print info messages.
# $@ := description files_to_check
check() {
    local description="${1:?} in: ${check_folders[*]/%/\/}"
    local -a check_files=("${@:2}")

    if (("${#check_files[@]}" > 0)); then
        echo "Checking for $description" >&2
        case "${report:?}" in
           1) exec pylint "${pylint_options[@]}" -fparseable --exit-zero \
               "${check_files[@]}" >"${report_file:?}";;
           *) exec pylint "${pylint_options[@]}" "${check_files[@]}";;
        esac
    else
        echo "No $description" >&2
        [[ "$report" == 0 ]] || echo >"$report_file"
    fi
}

# $@ := script_arguments
main() {
    local -a check_files

    parse_cmdline "$@"

    case "$base_branch" in
        "") check "pylint findings" "${check_folders[@]}";;
        *)  read -ra check_files <<<"$(get_changed_files)"
            check "pylint findings in scripts changed relative to $base_branch" \
                  "${check_files[@]}";;
    esac
}

main "$@"
