<!-- Add brave-browser issue below that this PR will resolve -->
Resolves

<!-- CI-related labels that can be applied to this PR:
* CI/disable-pipeline-step-cache - instruct CI to not cache build steps between runs for the same commit hash
* CI/enable-coverage - enable coverage reporting for your code changes
* CI/enable-test-only-affected - instruct CI to only run tests affected by your change
* CI/run-audit-deps (1) - check for known npm/cargo vulnerabilities (audit_deps)
* CI/run-network-audit (1) - run network-audit
* CI/run-perf-smoke-tests - run smoke performance tests
* CI/storybook-url (1) - deploy storybook and provide a unique URL for each build
* CI/run-upstream-tests - run Chromium unit and browser tests on Linux and Windows (otherwise only on Linux)
* CI/skip-upstream-tests - do not run Chromium unit, or browser tests (otherwise only on Linux)
* CI/run-teamcity - run TeamCity
* CI/skip-teamcity - skip TeamCity
* CI/skip - do not run CI builds (except noplatform)
* CI/run-linux-arm64, CI/run-macos-x64, CI/run-windows-arm64, CI/run-windows-x86 - run builds that would otherwise be skipped
* CI/skip-linux-x64, CI/skip-android, CI/skip-macos-arm64, CI/skip-ios, CI/skip-windows-x64 - skip CI builds for specific platforms

(1) applied automatically when some files are changed (see: https://github.com/brave/brave-core/blob/master/.github/labeler.yml)
-->

<!--
## Checklist:

- Review design docs
  [Browser design principles](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/chrome_browser_design_principles.md)
  [Style guide](https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/c++.md)
  [Core principles](https://www.chromium.org/developers/core-principles/)
- Ensure there are (tests)[https://www.chromium.org/developers/testing/]. Unit test as much as possible (including edge cases), but also include browser tests covering high level functionality.
- Ensure that there are comments explaining what classes/methods are/do. The "why" is often more important than the "what" in comments. Also update any relevant docs (moving docs from wiki to brave-core if necessary).
- Request security or other review (third-party libraries, rust code, etc...) if applicable [security/privacy review is needed](https://github.com/brave/brave-browser/wiki/Security-reviews) [other review](https://github.com/brave/reviews/issues/new/choose)
  Also see [adding third-party libraries](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/adding_to_third_party.md) for general guidelines on using third party code
- Maks sure there is a [ticket](https://github.com/brave/brave-browser/issues) for your issue
- Use Github [auto-closing keywords](https://docs.github.com/en/github/managing-your-work-on-github/linking-a-pull-request-to-an-issue) in the PR description above
- Write a good [PR/commit description](https://google.github.io/eng-practices/review/developer/cl-descriptions.html)
- Squash any review feedback or "fixup" commits before merge, so that history is a record of what happened in the repo, not your PR
- Add appropriate labels (`QA/Yes` or `QA/No`; `release-notes/include` or `release-notes/exclude`; `OS/...`) to the associated issue
- Checked the PR locally:
  * `npm run test -- brave_browser_tests`, `npm run test -- brave_unit_tests` [wiki](https://github.com/brave/brave-browser/wiki/Tests)
  * `npm run presubmit` [wiki](https://github.com/brave/brave-browser/wiki/Presubmit-checks), `npm run gn_check`, `npm run tslint`
- Run `git rebase master` (if needed)
-->
