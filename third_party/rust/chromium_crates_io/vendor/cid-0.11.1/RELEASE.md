Release process
===============

Generating Changelog
--------------------

Install dependencies

```sh
$ npm install -g conventional-changelog-cli
$ cd rust-cid
$ conventional-changelog --preset angular
```

Add the output of that to `CHANGELOG.md`, and write a human-centric summary of changes.
Update the linked output to reference the new version, which conventional-changelog doesn't know about:

```md
# [](https://github.com/multiformats/rust-cid/compare/v0.9.0...v) (2022-12-22)
```
becomes:
```md
# [v0.10.0](https://github.com/multiformats/rust-cid/compare/v0.9.0...v0.10.0) (2022-12-22)
```

## Publishing

Publishing on crates.io, bumping version & generating tags is done using [`cargo-release`](https://github.com/crate-ci/cargo-release).

This requires the following permissions

- on github.com/multiformats/rust-cid
  - creating tags
  - pushing to `master`
- on crates.io
  - publish access to all published crates

Dry run

```sh
$ cargo release <patch|minor|major>
```

Actual publishing

```sh
$ cargo release --execute <patch|minor|major>
```
