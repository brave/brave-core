<!-- Add brave-browser issue below that this PR will resolve -->
Resolves

<!-- CI-related labels that can be applied to this PR:
* CI/run-audit-deps (1) - check for known npm/cargo vulnerabilities (audit_deps)
* CI/run-network-audit (1) - run network-audit
* CI/run-upstream-tests - run Chromium unit and browser tests on Linux and Windows (otherwise only on Linux)
* CI/run-linux-arm64, CI/run-macos-arm64, CI/run-windows-arm64, CI/run-windows-x86 - run builds that would otherwise be skipped
* CI/skip - do not run CI builds (except noplatform)
* CI/skip-linux-x64, CI/skip-android, CI/skip-macos-x64, CI/skip-ios, CI/skip-windows-x64 - skip CI builds for specific platforms
* CI/skip-upstream-tests - do not run Chromium unit, or browser tests (otherwise only on Linux)
* CI/skip-all-linters - do not run presubmit and lint checks
* CI/storybook-url (1) - deploy storybook and provide a unique URL for each build

(1) applied automatically when some files are changed (see: https://github.com/brave/brave-core/blob/master/.github/labeler.yml)
-->

## Submitter Checklist:

- [ ] I confirm that no [security/privacy review is needed](https://github.com/brave/brave-browser/wiki/Security-reviews) and no other type of reviews are needed, or that I have [requested](https://github.com/brave/reviews/issues/new/choose) them
- [ ] There is a [ticket](https://github.com/brave/brave-browser/issues) for my issue
- [ ] Used Github [auto-closing keywords](https://docs.github.com/en/github/managing-your-work-on-github/linking-a-pull-request-to-an-issue) in the PR description above
- [ ] Wrote a good [PR/commit description](https://google.github.io/eng-practices/review/developer/cl-descriptions.html)
- [ ] Squashed any review feedback or "fixup" commits before merge, so that history is a record of what happened in the repo, not your PR
- [ ] Added appropriate labels (`QA/Yes` or `QA/No`; `release-notes/include` or `release-notes/exclude`; `OS/...`) to the associated issue
- [ ] Checked the PR locally:
  * `npm run test -- brave_browser_tests`, `npm run test -- brave_unit_tests` [wiki](https://github.com/brave/brave-browser/wiki/Tests)
  * `npm run presubmit` [wiki](https://github.com/brave/brave-browser/wiki/Presubmit-checks), `npm run gn_check`, `npm run tslint`
- [ ] Ran `git rebase master` (if needed)

## Reviewer Checklist:

- [ ] A security review [is not needed](https://github.com/brave/brave-browser/wiki/Security-reviews), or a link to one is included in the PR description
- [ ] New files have MPL-2.0 license header
- [ ] Adequate test coverage exists to prevent regressions
- [ ] Major classes, functions and non-trivial code blocks are well-commented
- [ ] Changes in component dependencies are properly reflected in `gn`
- [ ] Code follows the [style guide](https://chromium.googlesource.com/chromium/src/+/HEAD/styleguide/c++/c++.md)
- [ ] Test plan is specified in PR before merging

## After-merge Checklist:

- [ ] The associated issue milestone is set to the smallest version that the
  changes has landed on
- [ ] All relevant documentation has been updated, for instance:
  - [ ] https://github.com/brave/brave-browser/wiki/Deviations-from-Chromium-(features-we-disable-or-remove)
  - [ ] https://github.com/brave/brave-browser/wiki/Proxy-redirected-URLs
  - [ ] https://github.com/brave/brave-browser/wiki/Fingerprinting-Protections
  - [ ] https://github.com/brave/brave-browser/wiki/Brave%E2%80%99s-Use-of-Referral-Codes
  - [ ] https://github.com/brave/brave-browser/wiki/Web-Compatibility-Exceptions-in-Brave
  - [ ] https://github.com/brave/brave-browser/wiki/QA-Guide
  - [ ] https://github.com/brave/brave-browser/wiki/P3A

## Test Plan:

