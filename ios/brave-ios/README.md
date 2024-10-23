This folder contains the contents of what used to be in the brave-ios repo.

## Building the Brave iOS app:

Unlike brave-core, the code inside this folder does not build using GN and `npm run build` commands.

1. Ensure you have brave-core fully set up already including running an `init` with the `--target_os=ios` argument supplied.
2. Open the `Client.xcodeproj` file in Xcode found in `ios/brave-ios/App`
3. Build and run the `Debug` scheme in Xcode

Debug & channel named schemes have pre-actions to build the rest of brave-core along with it

### Swift style
* Swift code should generally follow the conventions listed at https://github.com/raywenderlich/swift-style-guide

### Whitespace
* New code should not contain any trailing whitespace.
* We recommend *enabling* the "Automatically trim trailing whitespace" and keeping "Including whitespace-only lines" *deselected* in Xcode (under Text Editing).

### Attribution
The content of this folder was originally a fork of [Firefox iOS Browser](https://github.com/mozilla-mobile/firefox-ios)
