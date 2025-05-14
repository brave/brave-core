Release process
===============

Generating Changelog
--------------------

Install dependencies

```sh
$ npm install -g conventional-changelog-cli
$ cd rust-multihash
$ conventional-changelog --preset conventionalcommits
```

Add the output of that to `CHANGELOG.md`. Write a human-centric summary of changes and add migration instructions for breaking changes if needed.

Update the linked output to reference the new version, which conventional-changelog doesn't know about:

```md
# [](https://github.com/multiformats/rust-multihash/compare/v0.17.0...v) (2022-12-06)
```
becomes:
```md
# [v0.18.0](https://github.com/multiformats/rust-multihash/compare/v0.17.0...v0.18.0) (2022-12-06)
```

Create a pull request with the changelog changes and the correct version bumps to the crates.


Publishing
----------

Once the PR above is merged, the crate can be published. This is done using [`cargo-release`](https://github.com/crate-ci/cargo-release).

This requires the following permissions

- on github.com/multiformats/rust-multihash
  - creating tags
  - pushing to `master`
- on crates.io
  - publish access to all published crates

Dry run

```sh
$ cargo release --workspace
```

Actual publishing

```sh
$ cargo release --workspace --execute
```
