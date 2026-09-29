# SwiftMojo

Native Swift mojo bindings for iOS. SwiftMojo talks to Chromium C++ services
directly over mojo message pipes and ships as `SwiftMojo.framework` /
`SwiftMojo.xcframework`, alongside the existing Obj-C mojom wrappers
(`//brave/build/ios/mojom`).

Build it by setting `enable_swift_mojo = true` in your GN args, which adds
`:swift_mojo_xcframework` to `//brave/ios:brave_ios`.

## Layout

- `//brave/ios/swift_mojo`: the framework and xcframework targets.
- `//brave/ios/mojo/swift`: the Swift runtime (message encoding, routing,
  remotes and receivers) and its unit tests, which run as part of
  `ios_brave_unit_tests`.
- `//brave/ios/browser/mojo`: code compiled into BraveCore.framework, such as
  the mojo system shim SwiftMojo is built on.

## Rules

- Mojo is only reached through the shim in
  `//brave/ios/browser/mojo/brave_mojo_system.h`, which BraveCore exports and
  Swift imports as the `BraveMojoSystem` module. Never add deps on `//mojo`,
  `//base`, etc. to the runtime: they are already linked into BraveCore and
  would be statically linked a second time.
- `BraveMojoSystem` is imported with `internal import`, so none of its types may
  appear in SwiftMojo's public API.
- The framework ships a binary `.swiftmodule` only, so it must be consumed with
  the same Xcode version that built it.
- Everything is main actor bound for now.
