## Prerequisites

1. Clone [brave-browser](https://github.com/brave/brave-browser) if you haven't already
1. Make sure you meet [brave-browser's prerequisites](https://github.com/brave/brave-browser/wiki/macOS-Development-Environment)
1. Make sure you include [Brave NPM configs](https://github.com/brave/devops/wiki/npm-config-for-Brave-Developers)
3. Install Java [JDK](https://www.oracle.com/java/technologies/downloads/) and make sure it is in your `PATH` environment variable.
4. Run `npm install` if you haven't already
5. Make sure its up to date:
    ```shell
    cd brave-browser
    git checkout -- "*" && git pull
    npm run init -- --target_os=ios
    ```
    
### Keeping up to date after initial setup

Once you have set up using `init` you can then simply keep the `master` branch of `brave-core` up to date via git and run `sync` from now on to ensure you have the latest Chromium version.

```shell
cd /path/to/brave-browser/src/brave
git checkout master && git pull
npm run sync --target_os=ios
```

### Unit Tests

At the moment this doesn't support setting up a unit test bundle, so any changes to the tests files must still be ran using `npm run test brave_rewards_ios_tests -- --target_os=ios` as seen below

## Making Changes to BraveCore.xcframework

When you have changes that need to be fixed in the BraveRewards.framework (such as ledger or ads API), this needs to happen in brave-core.

1. Create your branch on brave-core:
    ```shell
    cd src/brave
    git checkout -b my-feature-branch
    ```
1. Make your changes to the BraveCore.xcframework files located in `ios`
    - Any files added or removed must be reflected in `BUILD.gn` (sources)
1. Build your changes by running an `npm run build` command with the target you want. A few examples:
    ```shell
    # Creates debug build
    npm run build -- Debug --target_os=ios
    # Creates iOS simulator debug build (required to run on Apple Silicon simulators)
    npm run build -- Debug --target_os=ios --target_arch=arm64 --target_environment=simulator
    # Creates release build
    npm run build -- Release --target_os=ios
    # Creates arm64 build
    npm run build -- Release --target_os=ios --target_arch=arm64
    ```
1. Run the tests:
    ```shell
    npm run test brave_rewards_ios_tests -- Debug --target_os=ios
    ```
1. Run dependency check using the arguments you provided in the build, for example:
    ```shell
    npm run gn_check -- Debug --target_os=ios --target_environment=simulator
    ```
1. Run format
    ```shell
    npm run format
    ```
1. Run linting:
    ```shell
    npm run lint
    ```
    
Failure to pass `gn_check`, `format` or `lint` will result in the `noplatform` CI job to fail. Ensure you run these before opening a PR.

### Testing in `brave-ios`

1. Copy xcframeworks to `brave-ios/node_modules/brave-core-ios` by running `build_in_core.sh ~/path/to/brave-browser`.
1. When things are working correctly, open a PR in brave-core and add all recommended reviewers.
    - Add auto-closing words to your PR description that references your original issue created in step 1 (i.e. `resolves https://github.com/brave/brave-browser/issues/9000`)
    - If your changeset does not affect the desktop build (i.e. no changes were made to non-ios files), make sure to add the appropriate CI labels to your PR *on creation*: `CI/skip-windows`, `CI/skip-windows-x86`, `CI/skip-linux`, `CI/skip-macos`, and `CI/skip-android`. This will cut the time waiting for CI to complete and save CI resources. If you forget to add these you can login to Jenkins and abort the build, add the labels, then restart the build.

### Updating BraveCore in `brave-ios`

1. Find the appropriate iOS build you need in [brave-browser/releases](https://github.com/brave/brave-browser/releases)
2. Copy the URL of the `brave-core-ios-{version}.tgz` asset found in that release
3. Update the URL in `package.json` and run `npm install`

