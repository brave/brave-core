#!/opt/homebrew/bin/bash
BB_ISSUE_PREFIX="https://github.com/brave/brave-browser/issues/"
BB_ISSUE_URL_REGEX='(http(s)?:\/\/)?github.com/brave/brave-browser/issues/\d+'
UPLIFT_PR_REGEX='uplift\s+(?:of)?:?\s+\K(#\d+)'
RESOLVE_ISSUE_REGEX="(?:resolv|fix|clos)(?:e|es|ed)?:?\s+\K(${BB_ISSUE_URL_REGEX})"
verlte() { printf '%s\n%s' "$1" "$2" | sort -C -V; }
normalize_issue_links() { gsed "s@\sbrave\/brave-browser\#\([0-9]\+\)@ ${BB_ISSUE_PREFIX}\1@g"; }

pr_body=$(gh pr view "${PR_NUMBER}" -R "${REPO_NAME}" --json body --jq .body | normalize_issue_links)

readarray -t issues_pr < <(echo "${pr_body}" | ggrep -ioP "${RESOLVE_ISSUE_REGEX}" || :)
echo "Looking for any uplifted PRs"
readarray -t prs < <(echo "${pr_body}" | ggrep -ioP "${UPLIFT_PR_REGEX}" || :)
readarray -t issues_uplift_pr < <(echo "${prs[@]}" | xargs -n 1 -I{} gh pr view {} -R brave/brave-core --json body --jq ".body" | normalize_issue_links | ggrep -ioP "${RESOLVE_ISSUE_REGEX}")
readarray -t issues < <(printf "%s\n" "${issues_pr[@]}" "${issues_uplift_pr[@]}" | sort -u | gsed '/^\s*$/d')
echo "Relevant issues detected:"
printf "%s\n" "${issues[@]}"

pr_milestone=$(gh pr view "${PR_NUMBER}" -R "${REPO_NAME}" --json milestone --jq .milestone.title)

for issue in "${issues[@]}"; do
    issue_milestone=$(gh issue view "${issue}" --json milestone --jq .milestone.title)
    if [[ -z "${issue_milestone}" ]] || verlte "${pr_milestone}" "${issue_milestone}"; then
        echo "Setting ${issue} milestone to ${pr_milestone}"
        echo gh issue edit "${issue}" -m "${pr_milestone}"
    fi
done
