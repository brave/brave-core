## Rust usage:

When integrating Rust code into brave-core, keep the following points in mind:

- Chromium’s networking stack must be used
- The deps being added (and the deps recursively being added) must be approved via an issue posted at [brave/reviews](https://github.com/brave/reviews/issues/new/choose)
- The new code does not duplicate things already done in Chromium
- https://chromium.googlesource.com/chromium/src/+/main/docs/adding_to_third_party.md#rust also applies generally to all Rust code in brave-core 
- Look at existing deps in `third_party/rust` and try to match up versions whenever possible. We are trying to avoid having multiple copies of the same third party libs
