Brave for iOS 🦁
===============

Download on the [App Store](https://itunes.apple.com/app/brave-web-browser/id1052879175?mt=8).

This branch (development)
-----------

This branch is for mainline development that will ship in the next release.

This branch currently supports iOS 11, and is written in Swift 4.

Please make sure you aim your pull requests in the right direction.

For bug fixes and features for the upcoming release, please see the associated milestone branch (e.g. *v2.1.3*).

Getting involved
----------------

We encourage you to participate in this open source project. We love Pull Requests, Bug Reports, ideas, (security) code reviews or any kind of positive contribution. Please read the [Community Participation Guidelines](https://www.mozilla.org/en-US/about/governance/policies/participation/).

* Discord:            [#iOS](https://discord.gg/cR3gmq5) for general conversing and [#developers-ios](https://wiki.mozilla.org/IRC) for development discussion.
* Bugs:           [File a new bug](https://github.com/brave/brave-ios/issues/new) • [Existing bugs](https://github.com/brave/brave-ios/issues)

Want to contribute but don't know where to start? Here is a list of [Good First Issues](https://github.com/brave/brave-ios/issues?q=is%3Aopen+is%3Aissue+label%3A%22good+first+issue%22).

Building the code
-----------------

> __As of April 2018, this project requires Xcode 9.3.__

1. Install the latest [Xcode developer tools](https://developer.apple.com/xcode/downloads/) from Apple.
1. Install Carthage
    ```shell
    brew update
    brew install carthage
    ```
1. Clone the repository:
    ```shell
    git clone https://github.com/brave/brave-ios.git
    ```
1. Pull in the project dependencies:
    ```shell
    cd brave-ios
    sh ./bootstrap.sh
    ```
1. Open `Client.xcodeproj` in Xcode.
1. Build the `Fennec` scheme in Xcode.

## Contributor guidelines

### Creating a pull request
* All pull requests must be associated with a specific Github issue.
* If a bug corresponding to the fix does not yet exist, please file it.
* Please use the following formats in your PR titles:
    <br>&nbsp;&nbsp;`Fix/Ref #<issueId>: <description>.`
    <br>&nbsp;&nbsp;Examples:
    <br>&nbsp;&nbsp;`Fix #102: Added Face ID usage description to plist.`
    <br>&nbsp;&nbsp;`Ref #102: Fixed type on Face ID usage description.`
* Add any additional information regarding the PR in the description.
* In the unlikely and rare situation that a PR fixing multiple, related issues separate issue numbers with a comma:
    <br>&nbsp;&nbsp;`Fix #159, Fix #160: Removed whitepsace for + button on right-side panel.`
* PRs will be squashed and merged, so it is important to keep PRs focused on specific tasks.

### Swift style
* Swift code should generally follow the conventions listed at https://github.com/raywenderlich/swift-style-guide.
  * Exception: we use 4-space indentation instead of 2.

### Whitespace
* New code should not contain any trailing whitespace.
* We recommend *enabling* the "Automatically trim trailing whitespace" and keeping "Including whitespace-only lines" *deselected* in Xcode (under Text Editing).
* <code>git rebase --whitespace=fix</code> can also be used to remove whitespace from your commits before issuing a pull request.

### Commits
* Each commit should have a single clear purpose. If a commit contains multiple unrelated changes, those changes should be split into separate commits.
* If a commit requires another commit to build properly, those commits should be squashed.
* Follow-up commits for any review comments should be squashed. Do not include "Fixed PR comments", merge commits, or other "temporary" commits in pull requests.
