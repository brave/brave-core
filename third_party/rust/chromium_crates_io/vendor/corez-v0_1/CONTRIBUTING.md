# Contributing to `corez`

Thank you for your interest in contributing!

## Code of Conduct

This project is governed by the Zcash
[Code of Conduct](https://github.com/zcash/zcash/blob/master/code_of_conduct.md).
By participating, you are expected to uphold this code.

## Questions

Ask in the `#libraries` channel of the
[Zcash R&D Discord](https://discordapp.com/channels/809218587167293450/876655911790321684).
You can also open a [GitHub issue](/issues).

## License

This project is dual-licensed under **MIT OR Apache-2.0**. Any contribution
intentionally submitted for inclusion in the work by you, as defined in the
Apache-2.0 license, shall be dual licensed as above, without any additional
terms or conditions.

## AI Contributions

See [AGENTS.md](AGENTS.md) for the AI contribution policy, including required
`Co-Authored-By` metadata and prior discussion on an issue of the proposed
change.

## Git Workflow

This project uses a **merge-based workflow** (no squash merges).

- Commits should represent discrete, logical changes.
- Keep commit history clean. We recommend
  [`git revise`](https://github.com/mystor/git-revise) for amending earlier
  commits in a branch.
- When a commit alters the public API or fixes a bug, it must also update
  `CHANGELOG.md` in the same commit.
- Commit messages: short title (< 120 chars), body explaining the motivation.

## Coding Style

### Type Safety

Type safety is paramount. Invalid states should be unrepresentable at the type
level. Use newtypes, private fields with safe constructors, and `enum`s with
semantic variants.

### Error Handling

Use `Result<T, E>` with descriptive error types. Do not panic or abort except in
provably unreachable cases. Allocation failure (OOM) panics from standard
collections are acceptable.

### Unsafe Code

This crate has a strict **no `unsafe` code** policy. The crate-level attribute
`#![forbid(unsafe_code)]` is enforced.

### Documentation

All public API items must have complete `rustdoc` doc comments.

### Formatting

Run `cargo fmt` before submitting. CI enforces `cargo fmt --all -- --check`.
