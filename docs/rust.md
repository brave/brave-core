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

### Testing an unreleased version of a dependency

`gnrt` generally only allows you to use published releases from https://crates.io for dependencies. For dependencies maintained by Brave, it may be desired to test releases on a local build before publishing an "official" release.

In this case, you can modify `brave/third_party/rust/chromium_crates_io/Cargo.toml` to add a patch section for your dependency. Then you can use a `path` or `git` directive to point to one of your local checkouts on disk, or a git repository, respectively.

Running `gnrt gen` will update your source tree to use the correct dependency versions. Note that the code from the vendored version of your crate will _not_ be updated (even when running `gnrt vendor`), you will still need to manually copy your code changes in.

#### Updating a patch version
[This PR](https://github.com/brave/brave-core/pull/20113/files) updates `adblock 0.8.0` to `adblock 0.8.1` without any `brave-core` code changes.

#### Updating a minor version
[This PR](https://github.com/brave/brave-core/pull/19648/files) updates `adblock 0.7` to `adblock 0.8`. This is generally similar to the patch version diff, but there are additional related changes (i.e. see `components/brave_shields/adblock/rs/BUILD.gn` where a path is changed), along with additional unrelated changes (`brave-core` code changes to account for the newer API).

## Review guidelines

A checklist for reviewing Rust code changes, including dependent library crates.
This is just a starting point and not a comprehensive list.

* Third-party dependency versions should be aligned with what's already in the upstream chromium or brave trees. If a change needs to update one of those crates, make sure there's at least an issue open for all other users to coordinate migration so we don't ship duplicate code.
* Carefully check any `thread_local` use. Leaf code may be called from different C++ threads in sequence, so thread-local variables won't be consistent.
* No IO or system calls are allowed
* Any use of `autocfg` in dependencies must be patched out and replaced by config settings in `BUILD.gn` based on variables there. Since we don't use cargo, most of these checks won't work, or will produce the wrong values. See [here](https://github.com/brave/brave-core/commit/29fc07ef291593b5c4b9b2587ca184ab3d890650) for an example handling these issues.
