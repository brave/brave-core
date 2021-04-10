#!/usr/bin/env bash
# Requirements: bash, coreutils, git, pylint.
# This is called via `npm run pylint' with the top of the brave-core repo as $PWD.

# Fail on any error.
shopt -s inherit_errexit
set -eEo pipefail

# Defaults. Overwritten in parse_cmdline() and set readonly.
check_folders=(build components installer script tools)
only_new=0
pylint_options=(-j0 -rn --rcfile=.pylintrc)
report=0
report_file=pylint-report.txt

# Print a help message and exit with 1 by default.
# $@ := [ exit_status ]
help() {
	cat <<-EOF
	USAGE: pylint.sh --help
	       pylint.sh [--compare-with <base_branch> [--only-new]] [--report]

	Run pylint on versioned python scripts in:
	${check_folders[*]/%/\/}

	--compare-with <base_branch>: only analyse files changed relative to base_branch
	--only-new: exclude old findings from <base_branch> (NOT IMPLEMENTED YET)
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
            --only-new) only_new=1;;
            --report) report=1;;
            --help) help 0;;
            *) error "Extraneous arguments passed: ${*:$i}";;
        esac
    done
    readonly base_branch check_folders only_new report report_file
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
    local description="${1:?}"
    local -a check_files=("${@:2}")

    if (("${#check_files[@]}" > 0)); then
        echo "Checking for $description in: ${check_folders[*]}" >&2
        case "${report:?}" in
           1) exec pylint "${pylint_options[@]}" -fparseable --exit-zero \
               "${check_files[@]}" >"${report_file:?}";;
           *) exec pylint "${pylint_options[@]}" "${check_files[@]}";;
        esac
    else
        echo "No $*" >&2
        [[ "$report" == 0 ]] || echo >"$report_file"
    fi
}

# $@ := script_arguments
main() {
    local -a check_files

    parse_cmdline "$@"

    # Decide which files to check and call check() to execute pylint.
    case "$only_new,$base_branch" in
        1,)  error "ERROR: The --only-new option is only valid with --compare-with";;

        1,*) #read -ra check_files <<<"$(get_changed_files)"
             #check "new pylint findings in scripts changed relative to $base_branch" \
             #      "${check_files[@]}"
             error "The --only-new option is not implemented yet";;

        0,)  check "pylint findings" "${check_folders[@]}";;

        0,*) read -ra check_files <<<"$(get_changed_files)"
             check "pylint findings in scripts changed relative to $base_branch" \
                   "${check_files[@]}";;
    esac
}

main "$@"
