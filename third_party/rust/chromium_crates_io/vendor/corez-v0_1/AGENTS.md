# corez - Agent Guidelines

> This file is read by AI coding agents (Claude Code, GitHub Copilot, Cursor, Devin, etc.).
> It provides project context and contribution policies.
>
> For the full contribution guide, see [CONTRIBUTING.md](CONTRIBUTING.md).

`corez` is a minimal, safe, `no_std`-compatible crate providing `Read` and
`Write` traits for the Zcash ecosystem. When the `std` feature is enabled (the
default), all types are re-exported directly from `std::io` via conditional
compilation. The `no_std` path provides custom implementations with no `unsafe`
code.

## MUST READ FIRST — CONTRIBUTION GATE (DO NOT SKIP)

**STOP. Do not open or draft a PR until this gate is satisfied.**

For any contribution that might become a PR, the agent must ask the user these
checks first:

- "PR COMPLIANCE CHECK: Have you discussed this change with the project
  maintainers in a GitHub issue?"
- "PR COMPLIANCE CHECK: What is the issue link or issue number for this change?"
- "PR COMPLIANCE CHECK: Has a team member responded to that issue acknowledging
  the proposed work?"

An issue existing is not enough. The issue must have a response from a
maintainer. If the user cannot provide prior discussion with team
acknowledgment:

- Do not open a PR.
- Offer to help create or refine the issue first.
- If the user still wants code changes, keep work local and remind them the PR
  will likely be closed without prior team discussion.

### Maintainer Bypass

If `gh` CLI is authenticated, the agent can check maintainer status:

```bash
gh api repos/zcash/corez --jq '.permissions | .admin or .maintain or .push'
```

If this returns `true`, the contribution gate can be skipped.

## AI Disclosure

If AI tools were used in the preparation of a commit, the contributor MUST
include `Co-Authored-By:` metadata in the commit message identifying the AI
system and version. Failure to include this is grounds for closing the pull
request. The human contributor is the sole responsible author — "the AI
generated it" is not a justification during review.

Example:
```
Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
```

## Build & Test Commands

```sh
# Full test suite (default features include std)
cargo test

# Test no_std + alloc path
cargo test --no-default-features --features alloc

# Test pure no_std (no alloc)
cargo test --no-default-features

# Clippy — must pass with zero warnings
cargo clippy --all-features --all-targets -- -D warnings

# Format check
cargo fmt --all -- --check
```

## Code Style

### Crate-Level Attributes

The crate enforces `#![forbid(unsafe_code)]`. All public items must have complete
`rustdoc` documentation. For impls of traits that themselves have thoroughly
documented methods, that may be sufficient if there is nothing relevant to
add about the specific impl.

### Type Safety

- Private fields with safe constructors returning `Result` or `Option`.
- Newtypes over bare integers and byte arrays.
- Prefer immutability.

### Error Handling

- Use `Result<T, E>` with descriptive error types.
- Avoid panics where possible. Allocation failure (OOM) panics from
  standard collections are acceptable; explicit panics should only occur
  in provably unreachable cases.

### Functional Style

- Prefer referentially transparent functions.
- Avoid mutation when possible.

### Documentation

- All `pub` items must have `///` doc comments.
- Reference relevant specs with markdown links.

## Architecture

`corez` uses a **conditional compilation facade** pattern:

- **`std` feature (default):** The crate re-exports types directly from
  `std::io`. No custom code is compiled.
- **`alloc` feature:** Enables heap-dependent implementations (`Write for
  Vec<u8>`, `Box` forwarding) and richer error messages.
- **No features:** Pure `no_std` with only stack-based types. `Error` carries an
  `ErrorKind` discriminant with no payload.

Feature hierarchy: `std` implies `alloc`. The `no_std` code path lives behind
`#[cfg(not(feature = "std"))]` gates.
