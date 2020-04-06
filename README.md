[![Build Status](https://travis-ci.org/brave/brave-ios.svg?branch=development)](https://travis-ci.org/brave/brave-ios)

Brave for iOS 🦁
===============

Download on the [App Store](https://itunes.apple.com/app/brave-web-browser/id1052879175?mt=8).

This branch (development)
-----------

This branch is for mainline development that will ship in the next release.

This branch currently supports iOS 12, and is written in Swift 5.

Please make sure you aim your pull requests in the right direction.

For bug fixes and features for the upcoming release, please see the associated [GitHub milestones](https://github.com/brave/brave-ios/milestones) (e.g. *2.1.3*).

Getting involved
----------------

We encourage you to participate in this open source project. We love Pull Requests, Bug Reports, ideas, (security) code reviews or any kind of positive contribution.

* Development discussion: ['Contributing-ios' Community Forums](https://community.brave.com/c/contributing/contributing-ios):
* Bugs:           [File a new bug](https://github.com/brave/brave-ios/issues/new) • [Existing bugs](https://github.com/brave/brave-ios/issues)

Want to contribute but don't know where to start? Here is a list of [Good First Issues](https://github.com/brave/brave-ios/issues?q=is%3Aopen+is%3Aissue+label%3A%22good+first+issue%22).

Building the code
-----------------

1. Install the latest [Xcode developer tools](https://developer.apple.com/xcode/downloads/) from Apple. (Xcode 11 and up required).
1. Make sure `npm` is installed, `node` version 12 is recommended
1. Install Carthage:
    ```shell
    brew update
    brew install carthage
    ```
1. Install SwiftLint:
    ```shell
    brew install swiftlint
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
1. Build the `Debug` scheme in Xcode.

Updating Rewards
----------------

Some Brave Rewards code is precompiled into a framework in the `BraveRewards` directory. It's code can be found in [brave-core](https://github.com/brave/brave-core/tree/master/vendor/brave-ios), and instructions on building and updating this code can be found [here](https://github.com/brave/brave-ios/blob/master/BraveRewards/Updating%20the%20BraveRewards%20framework.md)

## Contributor guidelines

### Creating a pull request
* All pull requests must be associated with a specific GitHub issue.
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

### Commits
* Each commit should have a single clear purpose. If a commit contains multiple unrelated changes, those changes should be split into separate commits.
* If a commit requires another commit to build properly, those commits should be squashed.
* Follow-up commits for any review comments should be squashed. Do not include "Fixed PR comments", merge commits, or other "temporary" commits in pull requests.

> In *most* cases Pull Request commits will remain intact with a merge commit on the targeted branch.

## Code Signing

1. After running the *bootstrap.sh* script in the setup instructions navigate to:
<br>`Client/Configuration/Local/DevTeam.xcconfig`
1. Add your *Apple Team ID* in this file:
<br>`LOCAL_DEVELOPMENT_TEAM = KL8N8XSYF4`

>Team IDs look identical to provisioning profile UUIDs, so make sure this is the correct one.

The entire `Local` directory is included in the `.gitignore`, so these changes are not tracked by source control. This allows code signing without making tracked changes. Updating this file will only sign the `Debug` target for local builds.

### Finding Team IDs

The easiest known way to find your team ID is to log into your [Apple Developer](https://developer.apple.com) account. After logging in, the team ID is currently shown at the end of the URL:
<br>`https://developer.apple.com/account/<TEAM ID>`

Use this string literal in the above, `DevTeam.xcconfig` file to code sign
