## Rust usage:

When integrating Rust code into brave-core, keep the following points in mind:

- Chromium’s networking stack must be used
- The deps being added (and the deps recursively being added) must be approved
  via an issue posted at [brave/reviews](https://github.com/brave/reviews/issues/new/choose)
- The new code does not duplicate things already done in Chromium
- https://chromium.googlesource.com/chromium/src/+/main/docs/adding_to_third_party.md#rust
  also applies generally to all Rust code in brave-core
- Look at existing deps in `third_party/rust` and try to match up versions
  whenever possible. We are trying to avoid having multiple copies of the same third party libs

### Patching crates

There can be cases where patching crates is necessary. To create a patch file,
make changes to a crate and create a patch with:

```sh
git format-patch \
    --start-number=101 \
    --src-prefix=a/brave/ \
    --dst-prefix=b/brave/ \
    --output-directory \
        third_party/rust/chromium_crates_io/patches/some-crate/ \
    HEAD^
```

For general instructions on how this works in upstream Chromium, check [these instructions](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/rust/chromium_crates_io/patches/README.md#steps-for-creating-new-patches).
