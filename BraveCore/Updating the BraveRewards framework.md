## Prerequisites

1. Clone [brave-browser](https://github.com/brave/brave-browser) if you haven't already
1. Make sure you meet [brave-browser's prerequisites](https://github.com/brave/brave-browser/wiki/macOS-Development-Environment)
1. Install Java [JDK](https://www.oracle.com/java/technologies/downloads/) and make sure it is in your `PATH` environment variable.
1. Run `npm install` if you haven't already
1. Make sure its up to date:
    ```shell
    cd brave-browser
    git checkout -- "*" && git pull
    npm run init -- --target_os=ios
    ```

## Setting up an Xcode work environment
 
You can now setup an Xcode workspace to make working on iOS code much easier.

Run one of the following command while at the root folder of brave-browser:
```shell
# Creates a debug target
npm run build -- Debug --target_os=ios --xcode_gen=ios
# Creates a release target
npm run build -- Release --target_os=ios --xcode_gen=ios
# Creates a device build target
npm run build -- Release --target_os=ios --target_arch=arm64 --xcode_gen=ios
```

Assuming you ran the first command, this will create a folder at `src/out/ios_Debug_Xcode` which will contain `Workspace.xcworkspace` with two projects:

- `Products`: This will be what you build. You will likely only ever be choosing the `BraveRewards.framework` target scheme. You can remain on this scheme and still have proper autocomplete
- `Sources`: This has all the sources of BraveRewards.framework found in its BUILD.gn and its dependencies. 
  
  If a file does not appear here, it is because it either:
  - Does not appear in the `sources` list in its accompanying BUILD.gn file, or
  - Is a generated file (such as mojo generated models)

  These files can still be navigated to by ⌘-clicking `#import`s even though they don't appear in the project after you have built the project at least once

**Important Note**: Edits to this Xcode project do not affect the ninja build, so if you add/remove a file, you must update BUILD.gn to reflect that.

### Debugging

Assuming you have created a debug simulator Xcode environment (`npm run build -- --target_os=ios --xcode_gen=ios`), you can debug both the test app and unit tests.

To debug the Brave iOS app attach to `Client` via Xcode, using `Debug` > `Attach to Process` or `Attach to Process by PID or Name` if you need the debugger attached immediately at startup

To debug unit tests, attach to `brave_rewards_ios_tests` via Xcode, using `Debug` > `Attach to Process by PID or Name` before running them via command below

### Unit Tests

At the moment this doesn't support setting up a unit test bundle, so any changes to the tests files must still be ran using `npm run test brave_rewards_ios_tests -- --target_os=ios` as seen below

## Making Changes to BraveRewards.framework

When you have changes that need to be fixed in the BraveRewards.framework (such as ledger or ads API), this needs to happen in brave-core.

1. Create your branch on brave-core:
    ```shell
    cd src/brave
    git checkout -b my-feature-branch
    ```
1. Make your changes to the BraveRewards.framework files located in `src/brave/vendor/brave-ios`
    - Any files added or removed must be reflected in `BUILD.gn` (sources)
    - Any directories added or removed must be reflected in `BUILD.gn` (include_dirs)
1. Build your changes either using Xcode or running one of the following commands:
    ```shell
    # This has to be run at brave-browser root dir
    cd ../..
    # Creates debug build
    npm run build -- Debug --target_os=ios
    # Creates release build
    npm run build -- Release --target_os=ios
    # Creates arm64 build
    npm run build -- Release --target_os=ios --target_arch=arm64
    ```
1. Run the tests:
    ```shell
    npm run test brave_rewards_ios_tests -- Debug --target_os=ios
    ```
1. Run dependency check:
    ```shell
    npm run gn_check -- Debug --target_os=ios
    ```
1. Run linting:
    ```shell
    npm run lint
    ```
1. Copy xcframeworks to `brave-ios/node_modules/brave-core-ios` by running `build_in_core.sh ~/path/to/brave-browser`.
    - If your PR is likely to take some time and your branch is on the brave-core remote you can edit `package.json` in brave-browser to point brave-core to your branch. This means any `npm run init` will set brave-core to your branch and not master.
1. When things are working correctly, open a PR in brave-core and add all recommended reviewers.
    - Add auto-closing words to your PR description that references your original issue created in step 1 (i.e. `resolves https://github.com/brave/brave-ios/issues/9000`)
    - If your changeset does not affect the desktop build (i.e. no changes were made to non-ios files), make sure to add the appropriate CI labels to your PR *on creation*: `CI/skip-windows`, `CI/skip-linux`, `CI/skip-macos`, and `CI/skip-android`. This will cut the time waiting for CI to complete and save CI resources. If you forget to add these you can login to Jenkins and abort the build, add the labels, then restart the build.
1. Upon your PR being merged by you or another member and assuming your `brave-ios` changeset requires the updated brave-core code, update your library using `./build_in_core.sh ~/path/to/brave-browser` again (this time ommiting skip-upgrade) and submit a PR on `brave-ios` with the changes related to your brave-core changes.
