# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 28.0.0 (2023-12-29)

### Chore (BREAKING)

 - <csr-id-18686dbd32e6920ab5d7271c32481f7f41eae4de/> upgrade `ratatui` and `crosstermion` to latest versions.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 21 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Upgrade `ratatui` and `crosstermion` to latest versions. ([`18686db`](https://github.com/byron/prodash/commit/18686dbd32e6920ab5d7271c32481f7f41eae4de))
</details>

## 27.0.0 (2023-12-07)

### New Features

 - <csr-id-6acf6fe22a9ec4f7a16626db556c051c2084ccb2/> Change duration formatting to be more human readable.
   Note that this changes duration output from something like `69d10h40m`
   to `69d 10h 40m`.

### Reverted (BREAKING)

 - <csr-id-b1fd37d272c59249c151e1dbf498d5e2767f5507/> All `termion`-related features are now removed and obsolete.
   After the most recent update, certion event-related features in crosstermion
   stopped working in the context of the GUI, so it's probably best to let it go.
   
   By now, `crosstermion` is also very much a more portable replacement.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 8 commits contributed to the release over the course of 44 calendar days.
 - 89 days passed between releases.
 - 2 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Thanks Clippy

<csr-read-only-do-not-edit/>

[Clippy](https://github.com/rust-lang/rust-clippy) helped 1 time to make code idiomatic. 

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v27.0.0 ([`967e99a`](https://github.com/byron/prodash/commit/967e99a034cf1f570864b068d9c00baa54290b19))
    - Merge branch 'replace-ansi_term' ([`3a50a18`](https://github.com/byron/prodash/commit/3a50a18551c98cb3e0fa8362062e0f6fd41d5d34))
    - All `termion`-related features are now removed and obsolete. ([`b1fd37d`](https://github.com/byron/prodash/commit/b1fd37d272c59249c151e1dbf498d5e2767f5507))
    - Thanks clippy ([`3c28eb0`](https://github.com/byron/prodash/commit/3c28eb04e42cf0b2d47e4a6538c2436e8abac274))
    - Upgrade to `crossterm` v0.27. ([`34397f1`](https://github.com/byron/prodash/commit/34397f1b39dca0d585326728681a70b654db7118))
    - Change duration formatting to be more human readable. ([`6acf6fe`](https://github.com/byron/prodash/commit/6acf6fe22a9ec4f7a16626db556c051c2084ccb2))
    - Refactor ([`d032106`](https://github.com/byron/prodash/commit/d0321067244ed3d76eedc0078c881af2ea603c5b))
    - Replace compound_duration with humantime This is the humantime part of PR https://github.com/Byron/prodash/pull/25 ([`f5143c9`](https://github.com/byron/prodash/commit/f5143c98be0f2f2267c4574805f81a61cd8d613a))
</details>

## 26.2.2 (2023-09-09)

This release relaxes trait-bounds of `Count`, `Progress` and `NestedProgress` to allow `?Sized` as well.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 5 commits contributed to the release.
 - 2 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v26.2.2 ([`bb1eadf`](https://github.com/byron/prodash/commit/bb1eadfc9042cc27b346cc064115a69163f10c05))
    - Prepare next release ([`414506d`](https://github.com/byron/prodash/commit/414506df859584d17eb8ec755534040b813fa953))
    - Merge pull request #24 from NobodyXu/fix/dyn ([`c905d5c`](https://github.com/byron/prodash/commit/c905d5cb8d62368bb4891cc732ce613d09d6498b))
    - Relax bound of `Progress`, `Count` impl for `DynNestedProgressToNestedProgress` ([`967ea46`](https://github.com/byron/prodash/commit/967ea46b71c2bb44f7cd75524d101c4bfd2df0bf))
    - Fix use of `DynNestedProgress` as trait object ([`7eb69ac`](https://github.com/byron/prodash/commit/7eb69ac3f4946c72a0598b3021153045f3e9626b))
</details>

## 26.2.1 (2023-09-06)

### Bug Fixes

 - <csr-id-53ea2b81292a5062816bb12af0ab4f7931d5d2fa/> Add missing forwardings for various methods.
   Not having these could lead to incorrect thoughput display.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v26.2.1 ([`f37161f`](https://github.com/byron/prodash/commit/f37161f49f65991fd97ad855cf23a4f0ace3c9bb))
    - Add missing forwardings for various methods. ([`53ea2b8`](https://github.com/byron/prodash/commit/53ea2b81292a5062816bb12af0ab4f7931d5d2fa))
</details>

## 26.2.0 (2023-09-05)

### New Features

 - <csr-id-b3cae191413653feef62808b60b7eea52145e55e/> add `BoxedProgress` type that implements `Progress`.
   This makes working with boxed progress even more flexible.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v26.2.0 ([`8f26902`](https://github.com/byron/prodash/commit/8f2690254576e32624abbfc4b5e18283e7e542f7))
    - Add `BoxedProgress` type that implements `Progress`. ([`b3cae19`](https://github.com/byron/prodash/commit/b3cae191413653feef62808b60b7eea52145e55e))
</details>

## 26.1.0 (2023-09-04)

### New Features

 - <csr-id-0705a734b401d44657be59e83e4cbf58cb4484a8/> add `progress::AtomicStep` to allow referring to it.
   Previously, only `StepShared` was available, which implies an `Arc`.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v26.1.0 ([`05bc923`](https://github.com/byron/prodash/commit/05bc92395aeeaf498ab46e479decf1ae71382835))
    - Add `progress::AtomicStep` to allow referring to it. ([`0705a73`](https://github.com/byron/prodash/commit/0705a734b401d44657be59e83e4cbf58cb4484a8))
</details>

## 26.0.0 (2023-09-04)

This release is all about making `dyn` possible both for nested progress, as well as for 'simple' one (previously known as `RawProgress`).
Switching to this release naturally makes it possible for users of `Progress` to also use `dyn Progress`, as this trait is now object safe (formerly `RawProgress`).
If there are compile errors, the code now needs `NestedProgress`, instead of `Progress`, and possibly the import of the `Count` trait.
Finally, it's recommended to review all usages of `Progress` as they can possibly be replaced with `Count` which provides the guarantee that only counting happens,
and no change of the progress information itself.

### New Features (BREAKING)

 - <csr-id-6aba6e34f3e39bba5e609a0bc780c758cb43c821/> split `Progress` into various super-traits to allow most of them to be dyn-safe.
   `Progress` is now `NestedProgress`, `RawProgress` is now `Progress`, and there is
   a new `Count` trait for solely counting things.
 - <csr-id-6c60835ae49487a220b1817bc6cea3ebc8bf1aff/> `Progress::counter()` is now mandatory.
   This should simplify downstream code and we just accept that we are dealing
   with a threaded world.
   This also comes with performance improvements as increments are now 250% faster.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 9 commits contributed to the release.
 - 13 days passed between releases.
 - 2 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v26.0.0 ([`6b94f28`](https://github.com/byron/prodash/commit/6b94f28102b3133e4c1cba3765b55108c528b534))
    - Merge branch 'simplify' ([`5e21df7`](https://github.com/byron/prodash/commit/5e21df796d4c8ddefd71ee1a35df3b591c6d5e58))
    - `Progress::counter()` is now mandatory. ([`6c60835`](https://github.com/byron/prodash/commit/6c60835ae49487a220b1817bc6cea3ebc8bf1aff))
    - Prepare release ([`e1e282a`](https://github.com/byron/prodash/commit/e1e282aa37fc753884407339196809f2b0b72d2d))
    - Fixup nested dyn-traits ([`5e76abf`](https://github.com/byron/prodash/commit/5e76abfbf8dc18afddea68873c50cce677450a54))
    - Merge branch 'feat/dyn-progress' into simplify ([`c1590e4`](https://github.com/byron/prodash/commit/c1590e4650a9ffcf96a216f6a9fe82a1cf7cc10e))
    - Split `Progress` into various super-traits to allow most of them to be dyn-safe. ([`6aba6e3`](https://github.com/byron/prodash/commit/6aba6e34f3e39bba5e609a0bc780c758cb43c821))
    - Add benchmarks for dyn-traits ([`9d03124`](https://github.com/byron/prodash/commit/9d03124667935314a9d4c8e52886b994967f2671))
    - Refactor ([`54094b6`](https://github.com/byron/prodash/commit/54094b63289446f0582c6b49a666b5d993625dff))
</details>

## 25.0.2 (2023-08-22)

<csr-id-05741765491984487beea7326eff9863b669ab51/>

### Chore

 - <csr-id-05741765491984487beea7326eff9863b669ab51/> Adjusting changelogs prior to release of prodash v25.0.2

### New Features

 - <csr-id-24d0b2aaa58978990fea90c2f3b387e238acf966/> Add new trait `DynProgress` & type `BoxedDynProgress`

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release over the course of 2 calendar days.
 - 37 days passed between releases.
 - 2 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v25.0.2 ([`abcc61b`](https://github.com/byron/prodash/commit/abcc61b456e75d4f700f3c476849d7c36c3ece15))
    - Adjusting changelogs prior to release of prodash v25.0.2 ([`0574176`](https://github.com/byron/prodash/commit/05741765491984487beea7326eff9863b669ab51))
    - Remove `atty` in favor of `is-terminal` ([`2bfe9ad`](https://github.com/byron/prodash/commit/2bfe9adca357cf48d7310036684eeb85efa5ef46))
    - Add new trait `DynProgress` & type `BoxedDynProgress` ([`24d0b2a`](https://github.com/byron/prodash/commit/24d0b2aaa58978990fea90c2f3b387e238acf966))
</details>

## 25.0.1 (2023-07-16)

### Bug Fixes

 - <csr-id-17dd8f64563dd96ca454494e24a7f229716f6b1f/> `log` progress now supports a shared counter, just like the tree-item implementation

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release.
 - 60 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v25.0.1 ([`3ad8226`](https://github.com/byron/prodash/commit/3ad8226fd7825669d843a9c1bde001de1b702049))
    - Merge branch 'log-fixes' ([`a38d22c`](https://github.com/byron/prodash/commit/a38d22cf10068ce4275d65eb3e74652c2977b7e2))
    - Upgrade criterion to the latest version, it compiles more quickly. ([`78272c0`](https://github.com/byron/prodash/commit/78272c0f2a243b9e07165813a0eacc6ade623b90))
    - `log` progress now supports a shared counter, just like the tree-item implementation ([`17dd8f6`](https://github.com/byron/prodash/commit/17dd8f64563dd96ca454494e24a7f229716f6b1f))
</details>

## 25.0.0 (2023-05-16)

### New Features

 - <csr-id-8941f4b5b9c0d00dfd7b82c756b128982f163a06/> Introduce the object-safe `RawProgress` trait.
   It's automatically implemented for `Progress` and allows for more flexible use
   of progress particularly in leaf nodes. This is useful if a function needs to take
   multiple types of progress as it is called from different places in the same function.
   
   Without dyn-traits, it's not possible to make such call.

### New Features (BREAKING)

 - <csr-id-84d96c7b6ab07462d6c20147958d5aa1a58a688e/> Make messaging functions thread-safe by taking shared borrow and requring `Sync`.
   That way it's possible to share the `RawProgress` object across threads and emit messages,
   much like a logging system that's more integrated with rendering.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 5 days passed between releases.
 - 2 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v25.0.0 ([`02fcb9b`](https://github.com/byron/prodash/commit/02fcb9bd40ff9c03a46ce68926995a9614328ae0))
    - Make messaging functions thread-safe by taking shared borrow and requring `Sync`. ([`84d96c7`](https://github.com/byron/prodash/commit/84d96c7b6ab07462d6c20147958d5aa1a58a688e))
    - Introduce the object-safe `RawProgress` trait. ([`8941f4b`](https://github.com/byron/prodash/commit/8941f4b5b9c0d00dfd7b82c756b128982f163a06))
</details>

## 24.0.0 (2023-05-11)

<csr-id-fe5d01736179271f6b7bf20367f5d0e2bb616c4a/>

### Chore (BREAKING)

 - <csr-id-fe5d01736179271f6b7bf20367f5d0e2bb616c4a/> switch from `tui` to `ratatui`.
   The latter is a maintained fork.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release.
 - 60 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Thanks Clippy

<csr-read-only-do-not-edit/>

[Clippy](https://github.com/rust-lang/rust-clippy) helped 1 time to make code idiomatic. 

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v24.0.0 ([`41ad0a4`](https://github.com/byron/prodash/commit/41ad0a45ea39e6b283f808768ea60495b3d2b65f))
    - Merge branch 'ratatui' ([`4920457`](https://github.com/byron/prodash/commit/492045793a23be9ebda7310d89593288a2bd3340))
    - Thanks clippy ([`25356a3`](https://github.com/byron/prodash/commit/25356a369c345b1e89f7c6c356ff7142020849de))
    - Switch from `tui` to `ratatui`. ([`fe5d017`](https://github.com/byron/prodash/commit/fe5d01736179271f6b7bf20367f5d0e2bb616c4a))
</details>

## 23.1.2 (2023-03-11)

### Bug Fixes

 - <csr-id-7966f79cc7009acb33761cee70398b05b0006cc1/> line renderer now properly detects changes.
   Previously change-detection was implemented based on the assumption that
   the progress tree is copied entirely. Now, however, the interesting values
   are shared.
   
   The change-detection was adjusted to keep the state's hash of the most recent
   drawing, instead of doing everything in line, which saves time hashing as well.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 9 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v23.1.2 ([`c15f8db`](https://github.com/byron/prodash/commit/c15f8db0a7221ad6326544685200b68cb9ff5ddd))
    - Line renderer now properly detects changes. ([`7966f79`](https://github.com/byron/prodash/commit/7966f79cc7009acb33761cee70398b05b0006cc1))
</details>

## 23.1.1 (2023-03-02)

A maintenance release without user-facing changes.

Most notably, `parking_lot` was upgraded to the latest version.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 2 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v23.1.1 ([`a91c52f`](https://github.com/byron/prodash/commit/a91c52f0407a8adf8f93ad66796e1979e08ce126))
    - Prepare changelog ([`1a4eb9b`](https://github.com/byron/prodash/commit/1a4eb9b5dea3e100b188be956ab7670f0b8d5ad6))
    - Upgrade dependencies, particularly `parking_lot` ([`7ae8a07`](https://github.com/byron/prodash/commit/7ae8a0793752b713c6605be45688ca81fbb7e75e))
</details>

## 23.1.0 (2023-02-28)

### New Features

 - <csr-id-6f966b4f859f1b02775dcb3461bacf46b46ab707/> improve performance of `progress::tree` operations by more than 50%.
   This was done by implementing shared state in a simple Mutex protected hashmap
   which for typical programs with less contention is faster than using the `dashmap`
   crate.
   
   However, for those who know they need it, the previous implementation is still available
   in with the `progress-tree-hp-hashmap` feature toggle.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release.
 - 61 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v23.1.0 ([`45b4b7e`](https://github.com/byron/prodash/commit/45b4b7ea06b08cb30bd1f1ca2c05874bb0cf19ce))
    - No need for `unsound-local-offset` anymore. ([`c53ab9d`](https://github.com/byron/prodash/commit/c53ab9d5d238be4aaabfb8a990c80ca1a7c58dec))
    - Make fmt ([`490336f`](https://github.com/byron/prodash/commit/490336f9920ccde0ebe8d142dc51b390e9afc899))
    - Improve performance of `progress::tree` operations by more than 50%. ([`6f966b4`](https://github.com/byron/prodash/commit/6f966b4f859f1b02775dcb3461bacf46b46ab707))
</details>

## 23.0.0 (2022-12-29)

### New Features (BREAKING)

 - <csr-id-a1db1b27fc7f14be052bbfc660c9c9b174c1d3cc/> Implement `Hash` for `Task` to avoid redrawing if nothing changes with the Line renderer.
   That way, if everything stops due to a user prompt, the user's input won't be clobbered
   continnuously.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 23 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v23.0.0 ([`d527e4b`](https://github.com/byron/prodash/commit/d527e4be160ac80714deeaca11b8ed677f8d842f))
    - Implement `Hash` for `Task` to avoid redrawing if nothing changes with the Line renderer. ([`a1db1b2`](https://github.com/byron/prodash/commit/a1db1b27fc7f14be052bbfc660c9c9b174c1d3cc))
</details>

## 22.1.0 (2022-12-06)

### New Features

 - <csr-id-ea9aa5815ef2d1c734b85c185ddb65ac731b1195/> `progress::Key` now supports 6 levels of hierarchy instead of 4.
   That way it's less likely that surprises occour of more than necessary
   levels are added.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 1 day passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v22.1.0 ([`2ae17c5`](https://github.com/byron/prodash/commit/2ae17c5bd7ab91b9e05a16fb4f771ce0ce0001f4))
    - `progress::Key` now supports 6 levels of hierarchy instead of 4. ([`ea9aa58`](https://github.com/byron/prodash/commit/ea9aa5815ef2d1c734b85c185ddb65ac731b1195))
</details>

## 22.0.0 (2022-12-05)

<csr-id-46aeffd13cda49146c8a33e93c8c9b0fbcb15c8b/>

### Chore

 - <csr-id-46aeffd13cda49146c8a33e93c8c9b0fbcb15c8b/> switch to Rust edition 2021

### Changed (BREAKING)

 - <csr-id-53cb09dc0314b0e8ce58dc50e0c07a053b963ccd/> remove `Tree` and `TreeOptions` in favor of `tree::Root` and `tree::root::Options`.
   Previously it was confusing what a tree Root actually is due to the
   rename, and ambiguity isn't what we would want here.

### New Features (BREAKING)

 - <csr-id-edab37364276864d45241c2946173388a0602f23/> `From<tree::root::Options> for tree::Root`, `tree::root::Options::create()` returns `tree::Root` instead of `Arc`.
   That way we won't be forced to produce an `Arc` if it's not needed.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 5 commits contributed to the release.
 - 11 days passed between releases.
 - 3 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v22.0.0 ([`8b78fe9`](https://github.com/byron/prodash/commit/8b78fe9614a584096f4fefed38a8965266caf2e9))
    - Switch to Rust edition 2021 ([`46aeffd`](https://github.com/byron/prodash/commit/46aeffd13cda49146c8a33e93c8c9b0fbcb15c8b))
    - Make fmt ([`6e90cb9`](https://github.com/byron/prodash/commit/6e90cb965377762920d4dbb5debe8d47a4be89a2))
    - `From<tree::root::Options> for tree::Root`, `tree::root::Options::create()` returns `tree::Root` instead of `Arc`. ([`edab373`](https://github.com/byron/prodash/commit/edab37364276864d45241c2946173388a0602f23))
    - Remove `Tree` and `TreeOptions` in favor of `tree::Root` and `tree::root::Options`. ([`53cb09d`](https://github.com/byron/prodash/commit/53cb09dc0314b0e8ce58dc50e0c07a053b963ccd))
</details>

## 21.1.0 (2022-11-23)

### New Features

 - <csr-id-c332a6f266a6ae0cacf19cb523e551bb63c1e7ea/> identify each progress item with `Id` using `add_child_with_id()`.
   An `Id` is four bytes like b"TREE" that are stable and
   identify progress items (as created by `add_child(…)` within
   a function call.
   
   Callers may use this knowledge to pick specific progress items
   for consumption, instead of trying to rely on identifying tasks
   by name which may change.
   
   The identifier can also be queried with `Progress::id()`, even
   though it could be `prodash::progress::UNKNOWN` if the progress
   item wasn't created with `add_child_with_id()`.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release.
 - 37 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v21.1.0 ([`6327e15`](https://github.com/byron/prodash/commit/6327e1587d6886f4279ec037ad587dab52a7cedf))
    - `make fmt` ([`5415acc`](https://github.com/byron/prodash/commit/5415acc4cae7bab81a22758fe969bdb8b3deac13))
    - Make it easy to format 'use'es in the codebase. ([`5b8e54d`](https://github.com/byron/prodash/commit/5b8e54d959f895d762a4a77d194f3947a6302783))
    - Identify each progress item with `Id` using `add_child_with_id()`. ([`c332a6f`](https://github.com/byron/prodash/commit/c332a6f266a6ae0cacf19cb523e551bb63c1e7ea))
</details>

## 21.0.0 (2022-10-17)

### New Features

 - <csr-id-3347a0294c5b95270c34de9f396214353baea36d/> `impl Progress for &mut T: where T: Progress`.
   This makes it possible to hand borrowed progress implementations to
   functions that need progress reporting, making the usage of progress
   easier.

### New Features (BREAKING)

 - <csr-id-300181bdd4b2ef1822dddd1fe814d7e3e5b26779/> remove `Progress: 'static` requirement.
   This requirement can be added where used and where needed, and
   originally snuck in because it was easier and `Progress` implementations
   typically are `'static` as well.
   
   However, that requirement made it impossible to implement `Progoress`
   for `&mut T where T: Progress`.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 27 days passed between releases.
 - 2 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v21.0.0 ([`d7fcf5b`](https://github.com/byron/prodash/commit/d7fcf5bb5a6bad8225c22ce6df848835f5790847))
    - `impl Progress for &mut T: where T: Progress`. ([`3347a02`](https://github.com/byron/prodash/commit/3347a0294c5b95270c34de9f396214353baea36d))
    - Remove `Progress: 'static` requirement. ([`300181b`](https://github.com/byron/prodash/commit/300181bdd4b2ef1822dddd1fe814d7e3e5b26779))
</details>

## 20.2.0 (2022-09-20)

### New Features

 - <csr-id-4904065a51594b5bc4f32a247e513b45093373fa/> Add `Progress::set_max()` to set the highgest expected progress value.

### Bug Fixes

 - <csr-id-414bc71e79475335345dbc529566a6efc3afee23/> don't reset the shared counter value on `init`.
   It's possible to re-initialize the progress, and when that's done
   it would detach the counter from previous instances that might have
   been observed by callers to `counter()`, which is surprising.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release.
 - 2 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v20.2.0 ([`f49f470`](https://github.com/byron/prodash/commit/f49f4703f5d09f9bbe8ec32bd249c42bd129d7ea))
    - Don't reset the shared counter value on `init`. ([`414bc71`](https://github.com/byron/prodash/commit/414bc71e79475335345dbc529566a6efc3afee23))
    - Add `Progress::set_max()` to set the highgest expected progress value. ([`4904065`](https://github.com/byron/prodash/commit/4904065a51594b5bc4f32a247e513b45093373fa))
    - Make more explicit what is cloned (without altering actual behaviour) ([`b023efd`](https://github.com/byron/prodash/commit/b023efdc9c076f15cef1978ad95d932f782bdea7))
</details>

## 20.1.1 (2022-09-20)

### Bug Fixes

 - <csr-id-a0e7da7d08331eeeea8ca0cb6e349fe32ce876bc/> implement `Progress::counter()` for all utility types.
   This was forgotten previously as there was a default implementation
   right from the start.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v20.1.1 ([`d152701`](https://github.com/byron/prodash/commit/d1527018e5156921c3263c9d9afafad8c488a3b9))
    - Implement `Progress::counter()` for all utility types. ([`a0e7da7`](https://github.com/byron/prodash/commit/a0e7da7d08331eeeea8ca0cb6e349fe32ce876bc))
</details>

## 20.1.0 (2022-09-20)

### New Features

 - <csr-id-08be317dbd77f0fdb9673b9c24c239ad0d5078c3/> `Progress::counter()` returns a shared step counter.
   This is useful if multiple threads want to access the same progress, without the need
   for provide each their own progress and aggregating the result.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release.
 - 5 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Thanks Clippy

<csr-read-only-do-not-edit/>

[Clippy](https://github.com/rust-lang/rust-clippy) helped 1 time to make code idiomatic. 

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v20.1.0 ([`6591872`](https://github.com/byron/prodash/commit/65918722da151b756f6a3facc3577bd2002feadc))
    - Fix compile warnings for some configurations ([`fe0ac06`](https://github.com/byron/prodash/commit/fe0ac0659c5faba9b9c0b699850ffc9e66b566f0))
    - Thanks clippy ([`deb7791`](https://github.com/byron/prodash/commit/deb77916c1ced591410aba2593284c6a1345d426))
    - `Progress::counter()` returns a shared step counter. ([`08be317`](https://github.com/byron/prodash/commit/08be317dbd77f0fdb9673b9c24c239ad0d5078c3))
</details>

## 20.0.1 (2022-09-15)

### Bug Fixes

 - <csr-id-80a4850802023e5a0f6fe85e6af416aaeb729e21/> Allow builds to succeed on Windows by not registering SIGWINCH signal.
   Without said signal, the render line will not automatically resize
   anymore, which in theory can be compensated for by re-obtaining
   the terminal dimensions every now and then (currently not done).

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 5 commits contributed to the release.
 - 2 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 1 unique issue was worked on: [#12](https://github.com/byron/prodash/issues/12)

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **[#12](https://github.com/byron/prodash/issues/12)**
    - Allow builds to succeed on Windows by not registering SIGWINCH signal. ([`80a4850`](https://github.com/byron/prodash/commit/80a4850802023e5a0f6fe85e6af416aaeb729e21))
    - Only register SIGWINCH signal on unix ([`5de765b`](https://github.com/byron/prodash/commit/5de765b6c134be6c439583dd89297fe2b3f1e41f))
    - Adjust CI configuration to catch more cross-platform errors ([`b7e0f2c`](https://github.com/byron/prodash/commit/b7e0f2c97bc6c384dfbb5ed390909a5a6850bb74))
 * **Uncategorized**
    - Release prodash v20.0.1 ([`b98595b`](https://github.com/byron/prodash/commit/b98595b8cc9fd2f0bf3ea8cc22fbd7cf6cb07ab1))
    - Do away with unsafe code by using safe wrappers instead. ([`447aa0f`](https://github.com/byron/prodash/commit/447aa0f518aaa97dd061813a501e6a0b8512dd88))
</details>

## 20.0.0 (2022-09-12)

<csr-id-a3b26782dc074c469b5fc480595d2ac9ef8bc9d0/>

### New Features

 - <csr-id-bab2ea09089e2eb8e4e826bb17211a444aa35f31/> line renderer adjusts when resizing the terminal.

### Chore (BREAKING)

 - <csr-id-a3b26782dc074c469b5fc480595d2ac9ef8bc9d0/> upgrade dependencies to tui `0.19` and crossterm `0.25`

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 175 days passed between releases.
 - 2 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v20.0.0 ([`54e0a6a`](https://github.com/byron/prodash/commit/54e0a6ae39ff53b5c6e7abe9f44d003b833bfed6))
    - Line renderer adjusts when resizing the terminal. ([`bab2ea0`](https://github.com/byron/prodash/commit/bab2ea09089e2eb8e4e826bb17211a444aa35f31))
    - Upgrade dependencies to tui `0.19` and crossterm `0.25` ([`a3b2678`](https://github.com/byron/prodash/commit/a3b26782dc074c469b5fc480595d2ac9ef8bc9d0))
</details>

## 19.0.1 (2022-03-20)

### Bug Fixes

 - <csr-id-dbca35f478253176e852c177e79b3253eaafe3bd/> line renderer will clear previous lines if progress is lost
   Previously it would just exit its main loop and leave lines on screen.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v19.0.1 ([`a0c88b7`](https://github.com/byron/prodash/commit/a0c88b7ce0541edd9b6f677454bc0fe694b16fc7))
    - Line renderer will clear previous lines if progress is lost ([`dbca35f`](https://github.com/byron/prodash/commit/dbca35f478253176e852c177e79b3253eaafe3bd))
    - Fix benchmark compilation ([`39bc237`](https://github.com/byron/prodash/commit/39bc2379c966244746b4ce361e576997159e19aa))
</details>

## 19.0.0 (2022-03-20)

### New Features

 - <csr-id-cba841c828142c0dd028dd9413c31f509f2bbb1b/> Improve render-log performance greatly.
   Previously it would check the current time each time somebody
   wants to log on any logger, greatly reducing performance as
   it would block on the mutex rust-std uses internally.
   
   Now we use a single thread to provide information about whether or not
   we may log, whose lifetime is bound to all of the log instances it
   governs.

### New Features (BREAKING)

 - <csr-id-b6d5245344bde92672cd98aecacb5d94ecca4e19/> Allow rendererers to respond to dropped progress roots
   Previously it needed extra effort to communicate that a the computation
   was done and the renderer should stop rendering progress.
   
   Now they only obtain a weak progress instance so it can drop if the
   computation is done, terminating it naturally and in time.
   
   Note that in case of the TUI, it would still be needed to respond
   to the GUI having shut down due to user request.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release over the course of 41 calendar days.
 - 41 days passed between releases.
 - 2 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v19.0.0 ([`fda577e`](https://github.com/byron/prodash/commit/fda577e72d831d455289a6786262ea94f861063e))
    - Improve render-log performance greatly. ([`cba841c`](https://github.com/byron/prodash/commit/cba841c828142c0dd028dd9413c31f509f2bbb1b))
    - Allow rendererers to respond to dropped progress roots ([`b6d5245`](https://github.com/byron/prodash/commit/b6d5245344bde92672cd98aecacb5d94ecca4e19))
    - Actually, the correct dashmap version is 5.1 ([`6bdb7e8`](https://github.com/byron/prodash/commit/6bdb7e8ea3a65d51e69436799de3f9862b55dba4))
</details>

## 18.0.2 (2022-02-07)

<csr-id-e4f2ab842b34f4a4fe9b2f4c34b664a2e3dba200/>

### Chore

 - <csr-id-e4f2ab842b34f4a4fe9b2f4c34b664a2e3dba200/> Upgrade dashmap to 5.0.1 (with security fix)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 5 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v18.0.2 ([`69f4295`](https://github.com/byron/prodash/commit/69f42953ec69da4c2c34c8c137dfe7b7a1c12598))
    - Upgrade dashmap to 5.0.1 (with security fix) ([`e4f2ab8`](https://github.com/byron/prodash/commit/e4f2ab842b34f4a4fe9b2f4c34b664a2e3dba200))
</details>

## 18.0.1 (2022-02-01)

### Bug Fixes

 - <csr-id-a1f8aa650d1a1d2ac53025e29c71782b1cab58c5/> Downgrade to dashmap 4.0
   While waiting for unoundness to be resolved.
   
   See the issue for details: https://github.com/xacrimon/dashmap/issues/167

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 9 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v18.0.1 ([`e9769dd`](https://github.com/byron/prodash/commit/e9769dd7ad5c6f4618dd138aea74aba10fba69b3))
    - Downgrade to dashmap 4.0 ([`a1f8aa6`](https://github.com/byron/prodash/commit/a1f8aa650d1a1d2ac53025e29c71782b1cab58c5))
</details>

## 18.0.0 (2022-01-23)

### New Features (BREAKING)

 - <csr-id-482b54f9c584b0e2d22c53622c9f7ad45b79ad2c/> upgrade to tui 0.17

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 19 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v18.0.0 ([`10531ae`](https://github.com/byron/prodash/commit/10531ae35570af6003275a92ba40612a5e23678f))
    - Prepare changelog ([`a7a58c0`](https://github.com/byron/prodash/commit/a7a58c0fb6ea1068b914a653eb33c96dc157dc06))
    - Upgrade to tui 0.17 ([`482b54f`](https://github.com/byron/prodash/commit/482b54f9c584b0e2d22c53622c9f7ad45b79ad2c))
</details>

## 17.0.0 (2022-01-03)

### New Features (BREAKING)

 - <csr-id-46214a306793a6a9f304f854dfd7396ceaf433d3/> Add `MessageLevel` parameter to `Progress::show_throughput_with(…, level)`
   This allows to use message level for highlighting of certain
   throughputs and results.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v17.0.0 ([`9085aaa`](https://github.com/byron/prodash/commit/9085aaaa0a7844c1d4b4afcec4b688f5c398ba87))
    - Add `MessageLevel` parameter to `Progress::show_throughput_with(…, level)` ([`46214a3`](https://github.com/byron/prodash/commit/46214a306793a6a9f304f854dfd7396ceaf433d3))
</details>

## 16.1.3 (2022-01-03)

### Bug Fixes

 - <csr-id-2fe3eebbd62ddd9beacc294eb6ec04b4b39a26f6/> `Progress::init(None, None)` now resets the progress entirely

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 2 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v16.1.3 ([`6551d45`](https://github.com/byron/prodash/commit/6551d45837aecc853f78e848b4257b559287a6ac))
    - `Progress::init(None, None)` now resets the progress entirely ([`2fe3eeb`](https://github.com/byron/prodash/commit/2fe3eebbd62ddd9beacc294eb6ec04b4b39a26f6))
</details>

## 16.1.2 (2022-01-01)

### Bug Fixes

 - <csr-id-aa70a27ef1de930fdd00266239ee262b52a681a1/> reset the shared value on init to avoid keeping the previously set value.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 4 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v16.1.2 ([`3f16e6e`](https://github.com/byron/prodash/commit/3f16e6e31ee2b6df70841900d5b118f6533f9d2e))
    - Reset the shared value on init to avoid keeping the previously set value. ([`aa70a27`](https://github.com/byron/prodash/commit/aa70a27ef1de930fdd00266239ee262b52a681a1))
</details>

## 16.1.1 (2021-12-27)

### Bug Fixes

 - <csr-id-ca5f544594facc92c8744b293c7287dcffe065e5/> correct signature of new 'running()' method

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v16.1.1 ([`f426c20`](https://github.com/byron/prodash/commit/f426c20dca5440f6d39c979a8a851ee444a2c388))
    - Correct signature of new 'running()' method ([`ca5f544`](https://github.com/byron/prodash/commit/ca5f544594facc92c8744b293c7287dcffe065e5))
</details>

## 16.1.0 (2021-12-27)

### New Features

 - <csr-id-3886754817ac528178b8ea326d0b4d576168eb28/> Setting the progress value is now 9x faster
   This is accomplished at the cost of not autoamtically setting the
   progress to 'running' anymore when the progress is set.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 8 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v16.1.0 ([`34ae001`](https://github.com/byron/prodash/commit/34ae0014627c5fde785691ffc6583c52b629da51))
    - Setting the progress value is now 9x faster ([`3886754`](https://github.com/byron/prodash/commit/3886754817ac528178b8ea326d0b4d576168eb28))
    - An experiment to show we don't want to rely on dashmap for this ([`4f527c1`](https://github.com/byron/prodash/commit/4f527c12caa85018de293762194f9a2aed5daaea))
</details>

## 16.0.1 (2021-12-19)

<csr-id-e6f53d59ef1aef027a2aad5b164535c6ca0d620b/>

### Chore

 - <csr-id-e6f53d59ef1aef027a2aad5b164535c6ca0d620b/> upgrade dashmap to latest version

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 6 commits contributed to the release over the course of 47 calendar days.
 - 109 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 1 unique issue was worked on: [#8](https://github.com/byron/prodash/issues/8)

### Thanks Clippy

<csr-read-only-do-not-edit/>

[Clippy](https://github.com/rust-lang/rust-clippy) helped 1 time to make code idiomatic. 

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **[#8](https://github.com/byron/prodash/issues/8)**
    - Run `cargo changelog --write` for an improved changelog ([`a1054e8`](https://github.com/byron/prodash/commit/a1054e89b6f5bbdead1a5c2f975cdce0da0700e7))
 * **Uncategorized**
    - Release prodash v16.0.1 ([`9418df2`](https://github.com/byron/prodash/commit/9418df2660e5cb17c9906f86fb379c0d22c7ddb7))
    - Upgrade dashmap to latest version ([`e6f53d5`](https://github.com/byron/prodash/commit/e6f53d59ef1aef027a2aad5b164535c6ca0d620b))
    - Cleanup changelog ([`5aa6275`](https://github.com/byron/prodash/commit/5aa627523536a85c382f0da20636963387b437bd))
    - Thanks clippy ([`c1258e2`](https://github.com/byron/prodash/commit/c1258e250207889c62ef3208590d84185752e1a2))
    - Looks like array syntax isn't supported anymore ([`bfbce01`](https://github.com/byron/prodash/commit/bfbce01af9b90f5ae6b0490d6d7cdc29e05a1338))
</details>

## v16.0.0 (2021-08-31)

### Improvements

- Use `time` version 0.3 when the `local-time` feature is enabled

### Breaking

- rename cargo feature `localtime` to `local-time`
- The `local-time` feature is not the default anymore, enable it using the `RUSTFLAGS="--cfg unsound_local_offset"` environment when building the binary.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v16.0.0 ([`fec0304`](https://github.com/byron/prodash/commit/fec0304eba34db15c981a040c21e0371a5ac50d2))
    - Improve docs ([`71d66ec`](https://github.com/byron/prodash/commit/71d66ecd2b3330759890f9056ea686c269fbe63a))
    - Upgrade to time 0.3 and opt in to unsound features for example binaries (unix only) ([`fb3e0b0`](https://github.com/byron/prodash/commit/fb3e0b035cbe911d72a544a779d46cdda7b8105c))
    - Rename 'localtime' to 'local-time' (cargo feature)… ([`b6ee809`](https://github.com/byron/prodash/commit/b6ee8096d22ce7a97c9bce9bcddad966a05b365f))
</details>

## v15.0.1 (2021-08-31)

* crosstermion is optional for some renderers (again)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 25 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Release prodash v15.0.1 ([`72ea8a9`](https://github.com/byron/prodash/commit/72ea8a9565da457d6f1881bc64e6223c57a951a0))
    - Fix manifest key! ([`d9275f2`](https://github.com/byron/prodash/commit/d9275f22c3845fb21f601d5d426a0949a398c47d))
</details>

## v15.0.0 (2021-08-05)

* Upgrade to TUI v0.16

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release over the course of 32 calendar days.
 - 50 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Thanks Clippy

<csr-read-only-do-not-edit/>

[Clippy](https://github.com/rust-lang/rust-clippy) helped 1 time to make code idiomatic. 

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Upgrade to tui 0.16 ([`7b45023`](https://github.com/byron/prodash/commit/7b45023eb724429b2e0ba8794d7e80f867b51456))
    - Thanks clippy ([`5ebf8b4`](https://github.com/byron/prodash/commit/5ebf8b4af0c19472a0efb29a2ac95dbe47c3fdd8))
    - Dependency update ([`30d61c5`](https://github.com/byron/prodash/commit/30d61c5f57ab9d1200d237a23d272a43f1609956))
    - Use pin instead of boxing unnecessarily. ([`aa715ae`](https://github.com/byron/prodash/commit/aa715ae0903cbc95a0e1245e5fa62e3b505eae35))
</details>

## v14.0.0 (2021-06-16)

* Swap `ctrlc` crate with `signal-hook` which is a must in library crates. `ctrlc` is only for applications
  who can control the single handler that it installs entirely.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 38 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - (cargo-release) version 14.0.0 ([`a6492c6`](https://github.com/byron/prodash/commit/a6492c697f6afa79ee9bdec7c355d7f4388e5a6c))
    - Prepare release ([`c2e3193`](https://github.com/byron/prodash/commit/c2e3193dab419c5bc94ee6138430248c0d36e299))
    - Use signal-hook instead of ctrlc ([`a733267`](https://github.com/byron/prodash/commit/a7332677625fecf18566a05498b5e9bbd2108bc6))
</details>

## v13.1.1 (2021-05-08)

* Fix compile error (and protect from that regression) if `render-line-autoconfigure` was enabled.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - (cargo-release) version 13.1.1 ([`d120f2f`](https://github.com/byron/prodash/commit/d120f2fb4f46fc66c4533d5c4f5f4ee1aeec7fd4))
    - Fix compile issue, prep for patch release ([`584768a`](https://github.com/byron/prodash/commit/584768a07b37d6c11928dc022c44cf2d5c2c7e08))
</details>

## v13.1.0 (2021-05-08)

* With the `render-line-autoconfigure` feature toggle, the new
  `Options::default().auto_configure(…)` method allows to adapt to the terminal/non-terminal autmatically.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - (cargo-release) version 13.1.0 ([`8aedcab`](https://github.com/byron/prodash/commit/8aedcab872bfbad27c4ca120370e428d1a227324))
    - Update changelog ([`ffd4a1f`](https://github.com/byron/prodash/commit/ffd4a1f15a4ca0872f535fc2460890a9328ba7fd))
    - Add render-line-autoconfigure feature toggle ([`212ce36`](https://github.com/byron/prodash/commit/212ce369c290a14e99be91e7d5cafb154154cf8b))
</details>

## v13.0.1 (2021-05-08)

* The line renderer won't try to hide the cursor if the output isn't a terminal.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 6 commits contributed to the release over the course of 5 calendar days.
 - 5 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Thanks Clippy

<csr-read-only-do-not-edit/>

[Clippy](https://github.com/rust-lang/rust-clippy) helped 1 time to make code idiomatic. 

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - (cargo-release) version 13.0.1 ([`0d41f11`](https://github.com/byron/prodash/commit/0d41f111875598a90b7a2a7a8f4e0872feb1a285))
    - Prepare point release ([`8bfd42b`](https://github.com/byron/prodash/commit/8bfd42b0b03fc2b108ec68ea925e3d27742789da))
    - Don't try to hide the cursor if the output isn't a terminal ([`26c7497`](https://github.com/byron/prodash/commit/26c74976ee4e6c3eec89dddf1a90e6ce22539dd8))
    - More robust example for handling ttys and TUI ([`4d1dd31`](https://github.com/byron/prodash/commit/4d1dd313430054f17b2daddad8bd9d81061e60b0))
    - Thanks clippy ([`13404f5`](https://github.com/byron/prodash/commit/13404f5f34d49e5607fa77e56bef0669b24c4adb))
    - Remove crosstermion, it now lives in https://github.com/Byron/tui-crates ([`f560f84`](https://github.com/byron/prodash/commit/f560f84436c1bd2b5af8b0af44e8e86200d22cc2))
</details>

## v13.0.0 (2021-05-02)

<csr-id-e3665a2100fba190fc0f047ff05f2904f4dcaf4a/>
<csr-id-c91d410e8d6242b78c44119155b5fc3b2956d111/>
<csr-id-03d1c2067778fb6ec231bf18dd587046a03434bc/>

* Upgrade to TUI v0.15

### Other

 - <csr-id-e3665a2100fba190fc0f047ff05f2904f4dcaf4a/> prep release
 - <csr-id-c91d410e8d6242b78c44119155b5fc3b2956d111/> prepare release
 - <csr-id-03d1c2067778fb6ec231bf18dd587046a03434bc/> Upgrade to tui 0.15

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 13 commits contributed to the release over the course of 109 calendar days.
 - 110 days passed between releases.
 - 3 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - (cargo-release) version 13.0.0 ([`c0a97fa`](https://github.com/byron/prodash/commit/c0a97fa648cb2a0491d09bd0fbdd1daa6d6e290e))
    - Prep release ([`e3665a2`](https://github.com/byron/prodash/commit/e3665a2100fba190fc0f047ff05f2904f4dcaf4a))
    - Upgrade to tui 0.15 ([`edd11be`](https://github.com/byron/prodash/commit/edd11be0aa6ed84370603452286182a28c577961))
    - Prepare release ([`c91d410`](https://github.com/byron/prodash/commit/c91d410e8d6242b78c44119155b5fc3b2956d111))
    - Upgrade to tui 0.15 ([`03d1c20`](https://github.com/byron/prodash/commit/03d1c2067778fb6ec231bf18dd587046a03434bc))
    - Revert "Allow most recent version of 'time' crate" ([`ee8ab91`](https://github.com/byron/prodash/commit/ee8ab91f74d6e598f59947d08405ca1f7fdb2785))
    - Allow most recent version of 'time' crate ([`300aa7b`](https://github.com/byron/prodash/commit/300aa7b29ff6bad31197368340bebd2e2a5dfea0))
    - Run actions on main ([`a2b3037`](https://github.com/byron/prodash/commit/a2b303730602c1b6e4a63475b134f87d640b6d2f))
    - Make it more obvious what prodash actually is ([`5b862d5`](https://github.com/byron/prodash/commit/5b862d5e262cc08466afed235b862a6e79bffc8b))
    - Fix compile warning ([`d48f3b9`](https://github.com/byron/prodash/commit/d48f3b97643cbd3a264400cacff29eb1436cb002))
    - Use new resolver ([`ad03a43`](https://github.com/byron/prodash/commit/ad03a43056351899b8b76a62b0a0598b83c1e213))
    - Fix compile warnings ([`4cb8681`](https://github.com/byron/prodash/commit/4cb8681f3826994d92b703cc7cf105ccf01cc4d8))
    - Fix typo ([`75f311e`](https://github.com/byron/prodash/commit/75f311e5da2b5aeff608048d13075acb3dd41a0e))
</details>

## v12.0.2 (2021-01-12)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 3 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - (cargo-release) version 12.0.2 ([`29e16a4`](https://github.com/byron/prodash/commit/29e16a4faa4293f160b3294d327d2f7c4083ca2c))
    - Add all missing docs to prodash ([`473c560`](https://github.com/byron/prodash/commit/473c56048e762ca9976260598005e1508e8d579c))
</details>

## v12.0.1 (2021-01-08)

* upgrade dependencies

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 10 commits contributed to the release.
 - 4 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Thanks Clippy

<csr-read-only-do-not-edit/>

[Clippy](https://github.com/rust-lang/rust-clippy) helped 2 times to make code idiomatic. 

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - (cargo-release) version 12.0.1 ([`e93d001`](https://github.com/byron/prodash/commit/e93d00128cf24f63dcc18279bedea81d88732ef4))
    - Prepare release ([`3e58d82`](https://github.com/byron/prodash/commit/3e58d826b855f6f10b7a787d34288f84eea35e5f))
    - Fix localtime support ([`a3298d5`](https://github.com/byron/prodash/commit/a3298d5ab7e190d24ecdddc78f8c1cfcf5226bcb))
    - Fix time offset calculations which can (and do) indeed fail ([`fed01f7`](https://github.com/byron/prodash/commit/fed01f7bccc234534631341706527a47d19a6dfa))
    - Remove previous executor/reactor in favor of async-executor ([`1bea3cb`](https://github.com/byron/prodash/commit/1bea3cb16136c59bf9654fff16bcfefb1d0fa615))
    - Use new spawn for simple example ([`9a8af0f`](https://github.com/byron/prodash/commit/9a8af0fe86246cbc99bd8440529975189e77cc00))
    - Upgrade 'rand' ([`1c4930a`](https://github.com/byron/prodash/commit/1c4930af7e8960a94360f2ac6fdeae48920f7f93))
    - Thanks clippy ([`644809a`](https://github.com/byron/prodash/commit/644809a7e40fc3b49116e0dfad718fc4fa850164))
    - Upgrade dashmap and env_logger ([`cec8ab3`](https://github.com/byron/prodash/commit/cec8ab38513db79e588dc633218c565d5634f23f))
    - Thanks clippy ([`32be130`](https://github.com/byron/prodash/commit/32be1300186fce4d543861b850ca7adf7d8c76e2))
</details>

## v12.0.0 (2021-01-04)

* Upgrade to TUI v0.14

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 49 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Upgrade to tui 14 ([`06791b3`](https://github.com/byron/prodash/commit/06791b3cf0dd2589985bc1deb82b73e9278ae723))
    - Update to tui 14 ([`169d62d`](https://github.com/byron/prodash/commit/169d62d04f50eb1b97e6766eb8e7a8f7878b2aef))
</details>

## v11.0.0 (2020-11-15)

* Upgrade to TUI v0.13 and crossterm v0.18

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 10 commits contributed to the release over the course of 47 calendar days.
 - 59 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump major version ([`b2989fa`](https://github.com/byron/prodash/commit/b2989faa799f27701193c52c1b335302f9751639))
    - Upgrade to tui v0.13 ([`808f7d5`](https://github.com/byron/prodash/commit/808f7d5983ed9a3165afbd8f59959eca16d4a16a))
    - Crosstermion now uses tui v0.13 ([`7684a50`](https://github.com/byron/prodash/commit/7684a50e1b94d6f19fa7d2c4d646ff569a51ffbd))
    - Cargo clippy ([`725449b`](https://github.com/byron/prodash/commit/725449bfb3f4e39a543b023561b20365d9052a62))
    - Update README.md ([`b485568`](https://github.com/byron/prodash/commit/b4855683b4d33c4db2eade4db7a33a53de47961a))
    - Update README with accurate instructions on running example ([`d9288e9`](https://github.com/byron/prodash/commit/d9288e946bbc791f630c568b2b65899f644abd81))
    - Ignore interrupts when reading inputs… ([`a3bf8be`](https://github.com/byron/prodash/commit/a3bf8beeb8f9496fa102ff59aeb4a7c6bdcac70e))
    - Update to crossterm 0.18, but… ([`b5aa292`](https://github.com/byron/prodash/commit/b5aa292bd89cb223e9239bad3decc74495363b55))
    - Update to tui 0.12 ([`a70e96d`](https://github.com/byron/prodash/commit/a70e96d64f52de0d3d4fcec9cbacbdf0fd6b7bb0))
    - Upgrade to tui 0.12 ([`0606d46`](https://github.com/byron/prodash/commit/0606d4639c286c1917b85aee02f294dbadbaba77))
</details>

## v10.0.2 (2020-09-17)

* Remove `futures-util` dependency

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 10 commits contributed to the release over the course of 3 calendar days.
 - 3 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - (cargo-release) version 10.0.2 ([`6e84aef`](https://github.com/byron/prodash/commit/6e84aef4df2d5d4c465abaa9b8a8a804f238e4ac))
    - Changelog update ([`75258ea`](https://github.com/byron/prodash/commit/75258ea37c17975b2359ee0fdf34836430a7dd08))
    - Remove dependency to futures-util ([`45b8dba`](https://github.com/byron/prodash/commit/45b8dbafd0d7e80d7169212ee4b1a0d696e8c4ec))
    - Switch to release version of futures-lite ([`4fda4c0`](https://github.com/byron/prodash/commit/4fda4c05e5f4b7be2ff4a3b8898b060aa5c49ed4))
    - Switch to latest version of futures-lite to remove stream::select_all ([`53ff638`](https://github.com/byron/prodash/commit/53ff638997736010f696db21c95c70cd87792ab0))
    - Get rid of some more future-util functionality - just one more missing ([`0c82556`](https://github.com/byron/prodash/commit/0c82556cba25c16bd50c7abc39f8bb8078ea4228))
    - (cargo-release) version 0.3.2 ([`e187a9d`](https://github.com/byron/prodash/commit/e187a9d4addcde2587f02fdffa1d7a06a58dd3a6))
    - Crosstermion without futures-util! ([`d41352d`](https://github.com/byron/prodash/commit/d41352dff8954cf64ed31c2644aba5c4d846256b))
    - (cargo-release) version 0.3.1 ([`0a6b6bc`](https://github.com/byron/prodash/commit/0a6b6bcb2ce629254e715ecdff5544303e7c2d2c))
    - Upgrade futures-lite dependency ([`da021ea`](https://github.com/byron/prodash/commit/da021ea8306eaadb2bd40b155d4c431a164b6c42))
</details>

## v10.0.1 (2020-09-13)

* upgrade dependencies to latest versions

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 5 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Thanks Clippy

<csr-read-only-do-not-edit/>

[Clippy](https://github.com/rust-lang/rust-clippy) helped 1 time to make code idiomatic. 

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - (cargo-release) version 10.0.1 ([`a401d55`](https://github.com/byron/prodash/commit/a401d55dffd8191308dcee55c3404d6e0d777ccd))
    - Thanks clippy ([`2ea963c`](https://github.com/byron/prodash/commit/2ea963cb7bf31f63fa7d64e7b6183f0cb73cbcc8))
    - Upgrade blocking ([`01db9af`](https://github.com/byron/prodash/commit/01db9af60103abc1c2dbc486943c5fe9878abb1b))
    - Upgrade async-io ([`4d71843`](https://github.com/byron/prodash/commit/4d71843120b68fa431628627b6b5458b3576b70a))
    - Upgrade to latest futures-lite ([`9eb47a6`](https://github.com/byron/prodash/commit/9eb47a671ea8befb7f16d805d9c7c57f0c72d5c2))
</details>

## v10.0.0 (2020-09-13)

### Breaking

* Enforce `Send + 'static` bounds for `Progress` trait.
   * This way it's clear that Progress is supposed to work in a threaded environment, which is the environment they are used in most often.
   * On the call site, this avoids having to specify these trait bounds explicitly.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 27 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Progress with 'Send + 'static' bounds; bump major version ([`50a90ec`](https://github.com/byron/prodash/commit/50a90ece86e9642cb8005a7b1472d29b4b14f197))
</details>

## v9.0.0 (2020-08-16)

### Breaking

* add `set_name(…)` and `name()` to `Progress` trait.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 4 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Add 'name' and 'set_name' methods to trait ([`94c4390`](https://github.com/byron/prodash/commit/94c4390af8cf2ba5c9fa03d6177cff72cb762ad8))
</details>

## v8.0.1 (2020-08-11)

Add missing trailing paranthesis in throughput display

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 1 day passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Add missing parenthesis; bump patch level ([`b2af5b9`](https://github.com/byron/prodash/commit/b2af5b938d2690d2feed23e9d02ce683c280aceb))
</details>

## v8.0.0 (2020-08-10)

<csr-id-66800fd4e6c9f517f19da4e26a75cb3f139353b0/>
<csr-id-64cfe9e87e038fb36492307dfb75cbc8204180d8/>

### New Features

* Provide various Units for improved formatting of durations, steps, counts and bytes, and specify display of percentages.
* Support for throughput display if enabled in _line_ and _tui_ renderer, and when opted in when creating units.
* Turn `ProgressStep` type into usize to allow for a broader range of values. Specifically, this enables counting bytes read and written
  of files or streams bigger than 2^32-1
* A new example program: **units**
* Add methods to help determine key hierarchy, useful for writing custom renderers: `Key::shares_parent_with(…)`,`Key::adjecency(…)`
* `Key::adjecency(…)`

### Breaking

* In `Progress::init(max, unit)` , `unit` now is `Option<Unit>`, existing code can be transformed using `progress.init(None, Some("label".into()))`.
* moved`tree::progress` into `progress` and renamed `Value` into `Task` and `Progress` into `Value`
* moved`tree::messages` into `crates::messages`
* moved`tree::key` into `crates::progress::key`
* moved `tree::Throughput` into `crate::Throughput`
* **removed** `deep_eq()` method in `Root` tree
* tui engine option `redraw_only_on_state_change` was removed without substitute
* **Move and rename**
  * `tree::ProgressState` → `progress::State`
  * `tree::Value` → `progress::Value`
  * `tree::Progress` → `Progress`
* Remove `Hash` implementation for all public types except for `tree::Key`
* Move `tui` and `line` renderers into the `render` module
* Rename `log-renderer` feature to `progress-tree-log`
* Rename `tui-renderer*` into `render-tui*` and `line-renderer*` into `render-line*`

### Other

 - <csr-id-66800fd4e6c9f517f19da4e26a75cb3f139353b0/> Attempt to impl throughput in display…
   …which can't work because it's actually never mutable due to the way
   drawing work: it operates on a snapshot, a copy, that is not written
   back.
   
   And even if it was, the type system statically concludes sync is needed
   as well for this to work.
   
   Long story short: No state changes are ever allowed with a system like
   this, and throughput needs to maintain just that.
   
   Throughput must be implemented in each renderer.
 - <csr-id-64cfe9e87e038fb36492307dfb75cbc8204180d8/> Try to manually implement/run a local executor

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 91 commits contributed to the release over the course of 19 calendar days.
 - 19 days passed between releases.
 - 2 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Thanks Clippy

<csr-read-only-do-not-edit/>

[Clippy](https://github.com/rust-lang/rust-clippy) helped 2 times to make code idiomatic. 

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - More precise throughput computation ([`360d3ee`](https://github.com/byron/prodash/commit/360d3ee664b21c126b087ada5cd897927e0389bc))
    - Remove throughput field which was effectively constant ([`52661ee`](https://github.com/byron/prodash/commit/52661eea38a511d3b1b47d63fd9ef9ac0f46caaf))
    - ThroughputOnDrop utility ([`a4ea34c`](https://github.com/byron/prodash/commit/a4ea34c5a0f5370e7d109abccab3872c4f38aa8d))
    - Provide a way to take out progress from a DropOrDiscard (for completeness) ([`2d57a54`](https://github.com/byron/prodash/commit/2d57a54c1508bc9314713384fe9cb08f949cbf76))
    - Make throughput printing more flexible, allowing to override value and unit ([`a80104c`](https://github.com/byron/prodash/commit/a80104c955675c6cc40adcd8b90885010bfc9d0a))
    - Format throughput utility with less arguments and using available Unit ([`0e84270`](https://github.com/byron/prodash/commit/0e8427029abb2d87244aa51669ea82086b545b81))
    - Fix benchmarks ([`7c5be6b`](https://github.com/byron/prodash/commit/7c5be6b62a85d8b06d1f2609203e38173903e611))
    - Make 'Log' more intuitive to reach ([`8cf7452`](https://github.com/byron/prodash/commit/8cf7452b795d0d1b7e16df75cf4a1cd4c3cbec3f))
    - Convenience functions for nicer use of render() ([`4b396f5`](https://github.com/byron/prodash/commit/4b396f5f49d879006801cd678a17628ec56f3d19))
    - Conform feature names for renderers ([`673d149`](https://github.com/byron/prodash/commit/673d149387b745944915ae1472e7a52bbd0d48de))
    - Conform feature name ([`8ba1e96`](https://github.com/byron/prodash/commit/8ba1e96702bccb36886fe99bb0fa5eb7c58af7b9))
    - Thanks clipppy ([`72bfa47`](https://github.com/byron/prodash/commit/72bfa47d468b2ecaf6086b62b2d1a25c860f1baa))
    - Log progress now works with units ([`37f1266`](https://github.com/byron/prodash/commit/37f1266131bf52b35e5c473305f4c1d07df468f2))
    - First bare addition of log progress (from gitoxide) ([`006ba9d`](https://github.com/byron/prodash/commit/006ba9db0345953ff2b76ec4f28f9b58a60c7ec1))
    - Feature toggle for dashmap backed tree ([`fa688c8`](https://github.com/byron/prodash/commit/fa688c80e7ba264c5b87ae06b9aadebacb0d823e))
    - Make tui renderer use Root trait ([`3da74b0`](https://github.com/byron/prodash/commit/3da74b0aff7c615911f0259336b7ec28a11249d1))
    - Use Root trait in line renderer ([`a977c44`](https://github.com/byron/prodash/commit/a977c446534ea7c18f037e2a51dcbc419aac7c3d))
    - Implement Root trait for tree::Root ([`ec5c673`](https://github.com/byron/prodash/commit/ec5c6732098f9f90ba62184aacdd76f397d3d8a8))
    - Allow general use of Throughput by moving it into the top-level ([`cdabdce`](https://github.com/byron/prodash/commit/cdabdceb7ba9db76630f0cf58a7a4e2d4cc03f6f))
    - Move `tree::key` into `progress::key` ([`d6f66b7`](https://github.com/byron/prodash/commit/d6f66b785a12f3972c29391df8a8d842cfed2a46))
    - Refactor ([`63fd65a`](https://github.com/byron/prodash/commit/63fd65a82c57af25023ee9615662481b49e1d4e3))
    - First stab at sketching out the Root trait - it needs a key, however :D ([`08d30c6`](https://github.com/byron/prodash/commit/08d30c61f12aa69a66e46639016e4c9c15300a20))
    - Upgrade `init` to support the `Unit` type ([`226a849`](https://github.com/byron/prodash/commit/226a84944c8caba5aa7abac99e3d815b33d5f935))
    - Add `Progress` trait and implement it for `tree::Item` ([`93ffb60`](https://github.com/byron/prodash/commit/93ffb6037bec833fdf2d0bc51bbe6632d06aa17a))
    - Rename `Value` -> `Task` and `Progress` -> `Value` ([`f021ed8`](https://github.com/byron/prodash/commit/f021ed8f26faa360bfbd68b3a4479540dcea2942))
    - Move `tree::messages` into `crate::messages` ([`4bce1d7`](https://github.com/byron/prodash/commit/4bce1d742874dd2708f04f544d2979d8084c4667))
    - Moved tree::progress::* back into `crate::progress` ([`cf7405c`](https://github.com/byron/prodash/commit/cf7405c3d0ef6d6cbade68f68a1615a04e34bde8))
    - Prepare for generalizing the root interface to keep renderers general… ([`4e38c6b`](https://github.com/byron/prodash/commit/4e38c6bb1ea157fb067dd61f015e2cb0a2855c70))
    - Move most progress related types back to tree, however… ([`e6f242e`](https://github.com/byron/prodash/commit/e6f242e8555b2ef0fc15d6cf25273a991332f47c))
    - Move line and tui renderers into `render` module ([`31358a7`](https://github.com/byron/prodash/commit/31358a777b577772b28b9f58ba797d41fe5f4b00))
    - Thanks clippy ([`7435f35`](https://github.com/byron/prodash/commit/7435f35f9ddd88443eb87c0fe13256172d23d0f5))
    - Disable throughput for dashboard as it's not reqired ([`be39f49`](https://github.com/byron/prodash/commit/be39f49fa2e84139c6af2e870dcf079b234cabe6))
    - Remove Hash from types that don't need it ([`95e89ae`](https://github.com/byron/prodash/commit/95e89ae3ac18ca0e302b84695a1b6043ecbc0ee2))
    - Now it works! ([`2e91ed2`](https://github.com/byron/prodash/commit/2e91ed217f241290ffcb36c77e178f26393df422))
    - First attempt to bring throughput to TUI ([`d0e3af3`](https://github.com/byron/prodash/commit/d0e3af34ebf5f01bc1923e0024667200361a104b))
    - Refactor ([`9212de4`](https://github.com/byron/prodash/commit/9212de4662f7e35492a8796d5ef4dcece91e5ea8))
    - Throttling throughput recomputation to 1s really does the trick ([`6a4ae4e`](https://github.com/byron/prodash/commit/6a4ae4e7b933695e4c32359237c97cacc543ea8c))
    - First working example for throughput, but… ([`0ceebd4`](https://github.com/byron/prodash/commit/0ceebd4fa0e3b25e15cbeb3b68dddc76b28515e2))
    - First dummy throughput impl does what it should, nice! ([`e2ffb04`](https://github.com/byron/prodash/commit/e2ffb047b2eab3785b33db0925e7503d8e6300ea))
    - Possibly working impl of getting the throughput + reconcile ([`74315bf`](https://github.com/byron/prodash/commit/74315bf28e8451ef8008c6b98630b8984db38464))
    - Integrate calls to (optional) throughput into line renderer ([`a1af8fc`](https://github.com/byron/prodash/commit/a1af8fc78b7e5f8fd49b28df6352e9690caed8af))
    - First sketch of Thoughput handler ([`2edeefc`](https://github.com/byron/prodash/commit/2edeefca27592583186bca914b50e4bf6fc5f7e7))
    - Refactor ([`80684b3`](https://github.com/byron/prodash/commit/80684b3b60b069a6942b790c9b308649f432dbc4))
    - Refactor ([`19e0901`](https://github.com/byron/prodash/commit/19e090164be073351b16ded0a75168b48a5cf654))
    - Refactor ([`0402040`](https://github.com/byron/prodash/commit/040204078802f1d97a1a186f75d2e61cda3dd5d5))
    - Refactor ([`b981d2e`](https://github.com/byron/prodash/commit/b981d2eb055248ee67ada33da3b4ef946a842e8d))
    - Make key adjecency helpers public ([`3b12ea2`](https://github.com/byron/prodash/commit/3b12ea292468ed745d54267b7635b6b67ea84195))
    - Thanks clippy ([`a99e791`](https://github.com/byron/prodash/commit/a99e791427b5e2af4ad421545a650ca03c8f4e6c))
    - Refactor ([`c6068c5`](https://github.com/byron/prodash/commit/c6068c58bbcbf8faa28d9dc7d5ec3766ec41d953))
    - Refactor ([`983d2e5`](https://github.com/byron/prodash/commit/983d2e5c90773694dcd42fa54b5574a61621cf44))
    - Refactor ([`0c537ab`](https://github.com/byron/prodash/commit/0c537ab65af9d388e02fdea7175e7fbcd4fb4a3e))
    - Basic display of timespans for throughputs ([`fd68710`](https://github.com/byron/prodash/commit/fd687101407eced316b544dff48bb914acc5cf7f))
    - Refactor ([`40869a8`](https://github.com/byron/prodash/commit/40869a8322faa352c0d082f7ee33c2e533d9836d))
    - Prepare to move throughput calculations into renderer ([`682dee2`](https://github.com/byron/prodash/commit/682dee2ad35766c56e01458d0ec151587079ee4a))
    - Attempt to impl throughput in display… ([`66800fd`](https://github.com/byron/prodash/commit/66800fd4e6c9f517f19da4e26a75cb3f139353b0))
    - Pass elapsed time on in tui renderer ([`eb22417`](https://github.com/byron/prodash/commit/eb224176f041de36f9a6ab42888bceef0d3b85c5))
    - Pass elapsed time on in line renderer ([`33be555`](https://github.com/byron/prodash/commit/33be555f1e6c5141c5ee756a1d42b7ea0762f975))
    - Allow providing an optional elapsed duration… ([`a4b4ab7`](https://github.com/byron/prodash/commit/a4b4ab7d95457f7b662fb98d12eefb2bd0c34f39))
    - First test for throughput - before major refactoring ([`963c933`](https://github.com/byron/prodash/commit/963c933ebd2f5dcb090460dc79c445e122731c9a))
    - Remove unnecessary tui option: redraw_only_on_state_change ([`f78cf4f`](https://github.com/byron/prodash/commit/f78cf4fe8740b55f5c363151eab2ee1da558390c))
    - Prepare throughput display ([`3266fad`](https://github.com/byron/prodash/commit/3266fad2ad5e63855eb92c1f0c390bf2bfd8167f))
    - Make Unit a struct, introduce new 'Kind' to capture dynamic and static strings of display value ([`b413111`](https://github.com/byron/prodash/commit/b41311114ce3be7174dea7c1751fb15b20284794))
    - Prepare Mode to carry information about throughput handling ([`ea705ac`](https://github.com/byron/prodash/commit/ea705ac4089d7958e97b81f3ee1f8261e9948d2f))
    - Fix benches ([`a2d35fb`](https://github.com/byron/prodash/commit/a2d35fb096a915c0a55985f619a12149add29251))
    - Finish simple units example, showing all available units ([`de6addd`](https://github.com/byron/prodash/commit/de6addde16da64a7884eca8de221d398d868171c))
    - First simple progress bar using bytes ([`e3bdbf1`](https://github.com/byron/prodash/commit/e3bdbf1a8ef5c132405ec6570422b87ed07360ce))
    - Frame for new example program ([`2f1cb12`](https://github.com/byron/prodash/commit/2f1cb125b455c7357fd6576f3180e145c5efdc44))
    - Finally run unit-tests as well ([`70d7ae2`](https://github.com/byron/prodash/commit/70d7ae2bcdf98e60f83286cf12e53e0690893eef))
    - Refactor ([`6a18584`](https://github.com/byron/prodash/commit/6a185843344c1f54efc70f8bcf0e3736b6be87b0))
    - Use new Unit type everywhere ([`539edde`](https://github.com/byron/prodash/commit/539eddecb9d91d315771c8f30339c3b5cc186e18))
    - Support for nicer duration display ([`d500412`](https://github.com/byron/prodash/commit/d500412766c850133569c200cc13f6040a665af2))
    - Integrate humantime ([`f0f55bb`](https://github.com/byron/prodash/commit/f0f55bb6660523fe02cddc764a79fe2e3b459ce7))
    - Make range more compliant to aid consistent coloring ([`afb0c91`](https://github.com/byron/prodash/commit/afb0c91a20bd187f271791056238b0563015fbd1))
    - A new range display mode, only good with an upper bound ([`b83d6bd`](https://github.com/byron/prodash/commit/b83d6bd7137ea0dc8e9731552ae125cec6cb8631))
    - Support for omitting the unit ([`f350079`](https://github.com/byron/prodash/commit/f350079a9db9433936f0f90fe9316d0bed65cf38))
    - Using fmt::Write is so much better! ([`bf89a3d`](https://github.com/byron/prodash/commit/bf89a3d11fcc6eba087da1d28cace7991156bb39))
    - Trying to use byte-size shows that the trait interface isn't flexible enough ([`f233d43`](https://github.com/byron/prodash/commit/f233d43f2d48bb911cb20808396d4a2b0d55058d))
    - Allow splitting up generation of values and unit for more creative control… ([`0e15b97`](https://github.com/byron/prodash/commit/0e15b97d6a7cf5dcd7be068de8bf3f4b6472089c))
    - Percentage support ([`43c0980`](https://github.com/byron/prodash/commit/43c0980ffe15efe5e4994398d16b519d9e19836e))
    - First small steps towards implementing unit and value display correctly ([`5bd90cc`](https://github.com/byron/prodash/commit/5bd90cc6539bf61019de6d096589b1c2caeb685e))
    - Refactor ([`716e774`](https://github.com/byron/prodash/commit/716e77489aedd9df54745793c979af67acfbb41f))
    - Refactor in preparation for another example application ([`6063c27`](https://github.com/byron/prodash/commit/6063c2796b0b2508a73eb41b0c6c9ed8a601e21b))
    - Rough layout on how to get much more powerful unit and value rendering ([`90a9c2d`](https://github.com/byron/prodash/commit/90a9c2dac084b5ee9a60065875401c57644dce00))
    - First sketch for new Unit type and trait to display what we are interested in ([`129d09d`](https://github.com/byron/prodash/commit/129d09d6190619e64144a93d617b65f434d26f50))
    - Use 'usize' as ProgressStep, instead of u32 ([`79ae31f`](https://github.com/byron/prodash/commit/79ae31fc1dde5120a6e2706bc84995f72cc587dd))
    - Line renderer: more compact progress bar ([`8071110`](https://github.com/byron/prodash/commit/8071110e8a41612808206c3f8f444542818f4a51))
    - Revert "FAIL: Try to manually implement/run a local executor" ([`f6fa7ab`](https://github.com/byron/prodash/commit/f6fa7ab681549abfd77a14e4c8015f623f720c83))
    - Revert "Another attempt of using a local executor: FAIL" ([`a3254a6`](https://github.com/byron/prodash/commit/a3254a6aac6b4150bcbd10004d55d4cc06d45dbe))
    - Another attempt of using a local executor: FAIL ([`ee6275c`](https://github.com/byron/prodash/commit/ee6275ca3f0da906b9f94e6d95d9ce130907e9c8))
    - Try to manually implement/run a local executor ([`64cfe9e`](https://github.com/byron/prodash/commit/64cfe9e87e038fb36492307dfb75cbc8204180d8))
    - Actually only a few lines are needed to drive multi-task ([`d19a1db`](https://github.com/byron/prodash/commit/d19a1db08305abb77505868cc237b1c71491960d))
</details>

## v7.1.1 (2020-07-22)

* dependency update: smol 0.2

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Patch bump ([`33f57ef`](https://github.com/byron/prodash/commit/33f57efcd25523318d4c494dfd29d538be964992))
    - Upgrade to smol 2.0 ([`ab7dcb4`](https://github.com/byron/prodash/commit/ab7dcb4f2b108fbf7f525b61110f3f6b6339d0b0))
</details>

## v7.1.0 (2020-07-22)

* Improved looks thanks to bold fonts

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 5 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump minor version ([`aac8cb2`](https://github.com/byron/prodash/commit/aac8cb2ce99c3eb70b42919754e6ecefcab43854))
    - Use bold characters for some highlights ([`56cf67e`](https://github.com/byron/prodash/commit/56cf67e36ed0db6a4a78e1d30e7fc4eebe7c7342))
    - Upgrade to tui 0.10 ([`79afe12`](https://github.com/byron/prodash/commit/79afe123dda6e892425772fca8e040ac256edf9d))
    - Crosstermion 0.3 with support for tui 0.10 ([`dcda91b`](https://github.com/byron/prodash/commit/dcda91b744cb9f4f735591a53997ac4f2747a61e))
    - Decouple prodash from local crosstermion for tui migration ([`1105cfd`](https://github.com/byron/prodash/commit/1105cfd46d9a163c6e6d6c761fec8f70a0b08156))
</details>

## v7.0.4 (2020-07-21)

* **tree::Item**
  * Add new methods `inc_by(step)` and `inc()` for convenience 
* **line renderer**
  * They now look clearer, as they changed from \[===>     ] to \[===>------]

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 7 commits contributed to the release over the course of 1 calendar day.
 - 9 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - (cargo-release) version 7.0.4 ([`45a8a00`](https://github.com/byron/prodash/commit/45a8a00a76c3beed0baf0cffaa5656f0835a927e))
    - Improve line progress rendering ([`f694bb9`](https://github.com/byron/prodash/commit/f694bb995d6b3f3ef9c25b584bbd59bf7b625c7c))
    - Add convenience methods to tree::Item ([`3a36ce6`](https://github.com/byron/prodash/commit/3a36ce6082b3c6d24c8196ed4e9c537a5f36cb4f))
    - Prioritize the tui engine for futures-lite ([`c17235c`](https://github.com/byron/prodash/commit/c17235c493fffb628b15a730aa72cdd39f93ca12))
    - Attempt to switch to futures-lite, but a few things are missing ([`c8e52c2`](https://github.com/byron/prodash/commit/c8e52c2c045ddcd33f6f40d3f56ffbb3d00eb012))
    - Remove unused dependency ([`85c1f6d`](https://github.com/byron/prodash/commit/85c1f6d894f651a006793dfa0df3a547c8ed4a29))
    - Remove futures-util nearly completely - missing filter_map() extension for stream ([`6fc5846`](https://github.com/byron/prodash/commit/6fc58461028e63ca333adf7381178473329fe643))
</details>

## v7.0.3 (2020-07-11)

cleanup and code simplification in the line renderer.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump patch level ([`ffc58bb`](https://github.com/byron/prodash/commit/ffc58bbd0d305b4e3c1b33f5a6661cea4b692def))
    - Crosstermion 0.2 with much less code and complexity ([`f3b24d0`](https://github.com/byron/prodash/commit/f3b24d0edfdc40c5fb045facd467aab2e7e53f12))
    - Greatly simplify line renderer engine code ([`f0eaf27`](https://github.com/byron/prodash/commit/f0eaf279c42f0a7d28bc6b4c6d064df072f3a3a7))
</details>

## v7.0.2 (2020-07-11)

* **render-line** `JoinHandle` will 
  * now send a signal to perform a render before shutting down to capture the final state
  * wait for the render thread to complete the aforementioned actions on drop. You can still override this behaviour through
   `disconnect()` or `forget()`.
  * removed special code-paths that avoided bringing up another thread for 'ticks' at the expense of shutdown delay.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump patch level ([`9a7c2ef`](https://github.com/byron/prodash/commit/9a7c2efec958c5b0729e799177a94f5089882158))
    - Various improvements to help integrating with the line renderer ([`3711394`](https://github.com/byron/prodash/commit/371139409619f4a3195aaeabc8bf38a3b3ec6209))
</details>

## v7.0.1 (2020-07-10)

Prevent cursor movement if no progress bar is drawn.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump patch level ([`b1ca7fa`](https://github.com/byron/prodash/commit/b1ca7fa48d35a325566c90f49382b9385580be02))
    - Don't cause cursor movement when using '0' as MoveUp value ([`d090c0c`](https://github.com/byron/prodash/commit/d090c0ccbce3c4323eb03dc31fc550b7c11f2cf2))
    - Add new asciicast ([`443702b`](https://github.com/byron/prodash/commit/443702bf55c7e2677f867e1c5557dac0de78998b))
</details>

## v7.0.0 (2020-07-10)

<csr-id-a684188b3eee0cc67fe48b9ae14aa9cd63603caf/>
<csr-id-1bc5c764c9b1190f168d076b2183a27569750421/>

Add new render-line, change feature flag names.

### Other

 - <csr-id-a684188b3eee0cc67fe48b9ae14aa9cd63603caf/> first version of 'slow' event loop which actually won't respond quickly either :D
 - <csr-id-1bc5c764c9b1190f168d076b2183a27569750421/> bump patch level

### New Features

* **line**
   There is a new line renderer as neat trade off between bare logs and full-blown tui. It will work best with 'simple' and not
   too dynamically changing progress trees.
   Activate it with the `render-line` + one of `render-line-crossterm` or `render-line-termion` feature flags.
* Activate it with the `render-line` + one of `render-line-crossterm` or `render-line-termion` feature flags.

### Breaking Changes

* **`tui` module**
    * **TuiOptions** -> **Options**
    * `render_with_input` now takes the Write stream as argument, instead of defaulting to `std::io::stdout()`
* **Feature Flags**
    * **with-crossterm** -> **render-tui-crossterm**
    * **with-termion** -> **render-tui-termion**

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 81 commits contributed to the release over the course of 4 calendar days.
 - 4 days passed between releases.
 - 2 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Minior cleanup ([`48879f3`](https://github.com/byron/prodash/commit/48879f3cf44d1b8531819f7c4d3ca84d0bf116e9))
    - Dynamically resize and align progress bars - looks so much better now! ([`021dc5a`](https://github.com/byron/prodash/commit/021dc5abda66dad0c2270e5db093b3772e56e7b3))
    - Dynamically recompute dimensions of log messages ([`5cb7fea`](https://github.com/byron/prodash/commit/5cb7fea8e60b829ea32fec1c1a161a3cddb7cb5e))
    - Pass entire terminal dimensions to line buffer… ([`03a859c`](https://github.com/byron/prodash/commit/03a859c09f217c0d8e39b0ba7d2f05429fd92b16))
    - Allow setting the terminal width using CLI in dashboard example ([`bfd1a98`](https://github.com/byron/prodash/commit/bfd1a98990f9c7628bd82d6c1391dca8f783f570))
    - Always show log messages, but show progress only after initial delay. ([`7ec5530`](https://github.com/byron/prodash/commit/7ec5530ceed8c6b32b0a82fbe0f2d5e986d57a20))
    - Initial progress messages should actually be shown despite initial delay ([`4d68060`](https://github.com/byron/prodash/commit/4d6806028a780633e24966ca9900e0a517e6b783))
    - More fine-grained control over the amount of work displayed ([`9f4fa49`](https://github.com/byron/prodash/commit/9f4fa4938fac88ac2b70ebb96b52ddf7152cbd04))
    - Progress drawing now can be without color as well ([`f980a83`](https://github.com/byron/prodash/commit/f980a83226489dc1a1066a488aa39eb3c4d54cba))
    - Release notes ([`f1280a9`](https://github.com/byron/prodash/commit/f1280a9e18d3fc530cf7eb42eeb834d986c91477))
    - Nobody needs ticks right now, and using the step for unbounded progress rocks… ([`298afe6`](https://github.com/byron/prodash/commit/298afe685589827ab9a882f37ee53726486bd678))
    - Ascii-only unbounded progress ([`ab93db2`](https://github.com/byron/prodash/commit/ab93db261ec994ea8874c09015a1031ed91b70a1))
    - Refactor ([`a4aa7e8`](https://github.com/byron/prodash/commit/a4aa7e8fb894be910d0e297eb1be6c495641fd9f))
    - Reverse direction of unbounded progress by reversing an iterator, nice! ([`380330d`](https://github.com/byron/prodash/commit/380330d1e97011bc0ca1668d4686691b4a3986be))
    - A first, but backward, version of unbounded progress ([`89fb23c`](https://github.com/byron/prodash/commit/89fb23ca701eefa8fac9bb54697050be0ea30d28))
    - A somewhat usable display of bounded progress ([`8ebde51`](https://github.com/byron/prodash/commit/8ebde51451792a0b45080e364db7b840bece0282))
    - Prepare nicer but non-unicode drawing to realize we really want to configure the width ([`54e1ea7`](https://github.com/byron/prodash/commit/54e1ea77395d1da7a4bb5e91c47cc5949282780c))
    - Overdraw for log lines - works perfectly! ([`91ea381`](https://github.com/byron/prodash/commit/91ea381f8edbe9bdf2c19a2f0fdb5d4064bc19a3))
    - Make 'hiding the cursor' configurable ([`03a8a9e`](https://github.com/byron/prodash/commit/03a8a9ee9236a53deafdd341cd2dce18e32c05a8))
    - Fix 'make check' ([`a7bff83`](https://github.com/byron/prodash/commit/a7bff8372416f10d0ecb4175d20cb5b08529fe13))
    - Refactor ([`3a1b6c5`](https://github.com/byron/prodash/commit/3a1b6c5726c12a0051e255563aeb822af298f98d))
    - Refactor ([`fb91dde`](https://github.com/byron/prodash/commit/fb91dde5cf89d3efc44d554ee1eb433d72b5d542))
    - Show and hide the cursor if the ctrl+c is pressed (or SIG_TERM is sent) ([`ff98952`](https://github.com/byron/prodash/commit/ff98952c1f0310d32e580d5ba56d4615b64a24ed))
    - Refactor ([`dcb24ea`](https://github.com/byron/prodash/commit/dcb24eaac1f31bb07bc3fea5a8893018c7b220b6))
    - Use a vecdeque as it is just perfect for what we need to do here ([`8b7dbaa`](https://github.com/byron/prodash/commit/8b7dbaa9cbd2b712ed98f9af5c0ed897f95f0ebd))
    - Basis for overdraw of log messages ([`7a4aecf`](https://github.com/byron/prodash/commit/7a4aecfef256ee4d4a21059a2ef35b010d3514e3))
    - Obtain terminal size from crossterm or termion, now the dashboard… ([`3484fed`](https://github.com/byron/prodash/commit/3484fedd0a7b20b065edda3c101b31b2f22fd880))
    - Support for termion version of the dashboard ([`a37cb2a`](https://github.com/byron/prodash/commit/a37cb2a03f832ee05c092c5bec0b481c1d922440))
    - Incorporate filtering into the line count logic - that works actually ([`e38e559`](https://github.com/byron/prodash/commit/e38e55954d3bfcb016e23dac4f24b866e59aa302))
    - Fix select issue, surprise. Crossbeam channels are certainly more robust ([`5be4885`](https://github.com/byron/prodash/commit/5be4885816d2ec2fccf2a0d2a01deb38e740edc4))
    - Make interval ticker actually work - looks like flume can't always select ([`af710d4`](https://github.com/byron/prodash/commit/af710d4391d8103ee03dd385285159bd1d61eff3))
    - Quite a failed attempt to move cursor back up for overdrawing… ([`04c686b`](https://github.com/byron/prodash/commit/04c686b6bec543e290b729b61ac69245953a4564))
    - First rough drawing logic without cursor movement ([`0134e0d`](https://github.com/byron/prodash/commit/0134e0d54357d4a541555d5749e91b55ede7a692))
    - Manually adjust fill length for correct results even with Chinese ([`4da38f2`](https://github.com/byron/prodash/commit/4da38f270c713ed2f6254154e06e655eee4dbae5))
    - Try to align messages a bit more nicely, but… ([`2744e64`](https://github.com/byron/prodash/commit/2744e643d91ec411814df84dd755a9ffd304a9c1))
    - Turn off timestamps by default ([`dd02770`](https://github.com/byron/prodash/commit/dd02770db79fc104b62a727cdf79266ca6f11298))
    - Draw time as well in line renderer ([`fa51fad`](https://github.com/byron/prodash/commit/fa51fad3e0467f25d949c1dfb02cb38d10bb322f))
    - Revert "Add time support to crosstermion - seems a bit out of place actually" ([`93397c7`](https://github.com/byron/prodash/commit/93397c7f89e9c3d790cf2552690de1ef66f9e2d1))
    - Make 'time' module available to be shared by multiple renderers ([`c31d1c5`](https://github.com/byron/prodash/commit/c31d1c5adf6f83d0ffd44d91d8032f835038bb60))
    - Add time support to crosstermion - seems a bit out of place actually ([`8e2bf7c`](https://github.com/byron/prodash/commit/8e2bf7cceb4f96d9fad523944868102951be094d))
    - Move conditional painting code into crosstermion - could be useful for dua as well ([`6d04514`](https://github.com/byron/prodash/commit/6d04514d9015d6dcfec7688d6deab06b6ac33f54))
    - First sketch of conditional drawing of log messages ([`78f56c0`](https://github.com/byron/prodash/commit/78f56c0ffa4bea77a6ab7458734ad2cd206ac3e8))
    - Add a new 'color' module for crosstermion ([`272a852`](https://github.com/byron/prodash/commit/272a8525f5a102680f620d31d953467beae0b485))
    - Fix division by zero when copying the entire message buffer ([`60f1bb8`](https://github.com/byron/prodash/commit/60f1bb8cb47b6f0720b064db811416eea52313bb))
    - Cargo clippy ([`20f3144`](https://github.com/byron/prodash/commit/20f31449ae8e0c9de8c72f9286a7174a05972585))
    - Add support for no-line-color flag; implement additional line renderer options ([`eda7fb3`](https://github.com/byron/prodash/commit/eda7fb32c8c621545824ecb7c37e7bfdd96cb3d3))
    - Make coloring configurable, following some informal specs ([`1e1a02a`](https://github.com/byron/prodash/commit/1e1a02ab2c82ab332d5cbc8d5966810574cced8d))
    - Integrate message copying into line renderer ([`79efb09`](https://github.com/byron/prodash/commit/79efb094c92bb99eae77391dd3ffc3551c79102b))
    - Copying only new messages seems to work now ([`982dfee`](https://github.com/byron/prodash/commit/982dfeedb10d32d469fc3424b74d9629a82c3cd5))
    - Refactor ([`3bb08f7`](https://github.com/byron/prodash/commit/3bb08f707503c802a234c18e03af7ebcd578dff4))
    - Initial mostly working version of copying only new messages from the buffer. ([`d78472c`](https://github.com/byron/prodash/commit/d78472c579bfa20f4b2cc3fd2a062dac330dfdbf))
    - Allow for a little more space when formatting :) ([`4bf4431`](https://github.com/byron/prodash/commit/4bf44310c9e1c37644e07092e1c5c80c9e6c45d4))
    - First beginnings of testing copy_new(…) ([`917dd05`](https://github.com/byron/prodash/commit/917dd05122c39cd065eb5b7b761d083bc003a86f))
    - Now actually fix the 'copy_all(…)' method for the message buffer :D ([`2bb088d`](https://github.com/byron/prodash/commit/2bb088d48408dc2106dbe5ad2bed8db5699bba15))
    - Sketch for stateful message copying to copy only new ones. ([`90750c7`](https://github.com/byron/prodash/commit/90750c759a76d08b40b5e1b26d0d8d975afc1e2a))
    - Fix off-by-one error in messsage buffer copy handling ([`c6e1ece`](https://github.com/byron/prodash/commit/c6e1ecefff385e4acc56047654a527316f7fd5ac))
    - Refactor ([`ccc4297`](https://github.com/byron/prodash/commit/ccc4297a0e5dadca81a8398ae6495abbbf83cdd9))
    - Refactor ([`ed338e0`](https://github.com/byron/prodash/commit/ed338e05c97085e0e7673d148f6d7f382a88ef03))
    - Flesh out join handle with complete and symmetric API ([`1bd4476`](https://github.com/byron/prodash/commit/1bd4476b532425f0b7b2fdc82f058ed8b6b8317a))
    - Support for interruptable initial delay ([`c15af5e`](https://github.com/byron/prodash/commit/c15af5ef2fe7c1bfde74f6d183c22688f666f398))
    - Dashboard can now bring up the line renderer ([`f20f002`](https://github.com/byron/prodash/commit/f20f00282836f33465a372bf42ced1b04a3ca064))
    - Don't assume quitting is requested on channel disconnect; allow detaching the handle ([`d050243`](https://github.com/byron/prodash/commit/d050243e8176a35ec29905a047e1a69b44ddf0e0))
    - Frame to allow using 'line' renderer in dashboard example ([`4566e46`](https://github.com/byron/prodash/commit/4566e46155656e573af80fabd9df0ab6aec95531))
    - Make dashboard depend on line renderer ([`ea861a2`](https://github.com/byron/prodash/commit/ea861a26e3ca8f6db37a0da79465531c2299c926))
    - Now we are talking: Selector functions shouldn't have side-effects, 'wait()' is exactly it ([`9ebbc16`](https://github.com/byron/prodash/commit/9ebbc1685e4f7802cb6384711a06199d9cd8a58a))
    - First version of 'slow' event loop which actually won't respond quickly either :D ([`a684188`](https://github.com/byron/prodash/commit/a684188b3eee0cc67fe48b9ae14aa9cd63603caf))
    - Sketch draw logic for draw loops that are fast enough ([`b4db64e`](https://github.com/byron/prodash/commit/b4db64e3a134cea153db92ccb19dbc20d9ab9ee0))
    - Sketch initial interface for line renderer ([`1712221`](https://github.com/byron/prodash/commit/1712221f04efb20717dbe662a8d29d4a23235989))
    - Move changelog information into its own file ([`5aa3034`](https://github.com/byron/prodash/commit/5aa3034a2085fdf602641c7899448e30941183be))
    - Prepare arrival of the mighty line-renderer feature :D ([`511389e`](https://github.com/byron/prodash/commit/511389e7491d11e06815b86f41b594a9df7c0529))
    - Make external crates available as re-exports ([`0b8a763`](https://github.com/byron/prodash/commit/0b8a763585d7828f6ac1ccbcc23d985ff1eab3a9))
    - Notes about the tradeoffs in backend choice ([`a942e85`](https://github.com/byron/prodash/commit/a942e85b62118c447214f683f3c866e502842337))
    - Write down insights about coloring in terminals, in conjunction with crosstermion ([`c96abdb`](https://github.com/byron/prodash/commit/c96abdbc116801eb873e021151a853599f32ec43))
    - Bump patch level ([`c879dfa`](https://github.com/byron/prodash/commit/c879dfa59fa4de6f7b0a02de42668224d791e5db))
    - Fix description of crosstermion ([`8781c9f`](https://github.com/byron/prodash/commit/8781c9f3f70e12d436c957834f0a6dd61a74651b))
    - Fix precedence in terminal module, crossterm is winning over termion now ([`521dd23`](https://github.com/byron/prodash/commit/521dd23dc67d6dee52cfbe0c35c3fe8fc28dc881))
    - Bump patch level ([`1bc5c76`](https://github.com/byron/prodash/commit/1bc5c764c9b1190f168d076b2183a27569750421))
    - Make sure conversions are always compiled ([`5a03628`](https://github.com/byron/prodash/commit/5a03628b1cafd8d06476f364242d0e149f4fad18))
    - Fix flume features, bump to 0.1.2 ([`11d6665`](https://github.com/byron/prodash/commit/11d6665c77a1e4ce4dc262744ffb76dc8b907832))
    - Bump patch level; add 'input-thread-flume' support ([`7fdbb72`](https://github.com/byron/prodash/commit/7fdbb72822c450a4e13eba998236b608cf9eeca3))
    - Fix Cargo.toml to allow 'cargo test' to work without specifying features ([`748ab4b`](https://github.com/byron/prodash/commit/748ab4be6aa5fc975fcdcacbd92a2fa388103734))
</details>

## v6.0.0 (2020-07-05)

<csr-id-bbf2651e379b5758d53a889d9fb220c616d2a096/>

Factor terminal input into the new `crosstermion` crate.

Due to this work, the default features changed, which is a breaking change for those who relied on it.
Now when using the `render-tui`, one will also have to specify either the `with-crossbeam` or `render-tui-termion` feature.

### Other

 - <csr-id-bbf2651e379b5758d53a889d9fb220c616d2a096/> Add Key input transformation

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 24 commits contributed to the release.
 - 2 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Allow choosing the backend for the dashboard example, but people also have to chose is now ([`4841a8e`](https://github.com/byron/prodash/commit/4841a8ed3f949797990b0ad3a6148af4beb7203d))
    - Release notes for v6.0 ([`403d5a0`](https://github.com/byron/prodash/commit/403d5a0af5fbd072b876c61d59cbaba8c261b7df))
    - Fix crosstermion docs ([`71e1eca`](https://github.com/byron/prodash/commit/71e1eca154ccce82a012932c6b5f47cea57f4b9b))
    - Make feature combinations for properly, but… ([`93a069a`](https://github.com/byron/prodash/commit/93a069a2e6d9757f6ca2b77300f8c7d7baf091d4))
    - Cleanup; allow 'input-thread' and 'input-async' at the same time ([`93a30e0`](https://github.com/byron/prodash/commit/93a30e0baeeb59ea1b8f38e4a23c080c3942525f))
    - Cleaner input handling using 'input-threaded' and 'input-async' ([`3d11e90`](https://github.com/byron/prodash/commit/3d11e9040510ccbc43057432c9e537c49fd8025d))
    - Support for async crossterm in crosstermion ([`1a61453`](https://github.com/byron/prodash/commit/1a61453dd6e8f4c2eef7695e1e92285d318e9d9e))
    - Use stream trait instead of channel directly to abstract a little ([`78e810d`](https://github.com/byron/prodash/commit/78e810d4ee0cb691cec3666f4c585fffd6d6481b))
    - Don't continue the loop if sending fails… ([`e93fe6e`](https://github.com/byron/prodash/commit/e93fe6ebdf17abf38bba3e00a4c26ff822d42219))
    - Switch from smol to futures-executor, it's probably cheaper for what we need ([`d7deeb7`](https://github.com/byron/prodash/commit/d7deeb71983d08cb5998e22f23482a0776b7fb2f))
    - Cleanup, remove unused deps ([`050a821`](https://github.com/byron/prodash/commit/050a821a4f6d70ba624afdd48b744a62cb374254))
    - Fix prodash build after function renmae ([`3848635`](https://github.com/byron/prodash/commit/3848635432177f5cbc7ac9f18e7c9deadfeaf588))
    - Docs for crosstermion ([`3429327`](https://github.com/byron/prodash/commit/342932702dfc2ce359548eef72ff86d0b9030fbb))
    - Enforce testing termion on linux ([`0cce091`](https://github.com/byron/prodash/commit/0cce091c56fcda36a1ca5eb918bb67e48db6f2d9))
    - Prodash now uses `crosstermion` ([`97ad454`](https://github.com/byron/prodash/commit/97ad45420ad569682e3ee62d7bb7c28fe287b9e3))
    - Bundle-features and feature documentation ([`b66c413`](https://github.com/byron/prodash/commit/b66c4133fdec9b3cfc5a290898de9ce71f22e10c))
    - Marry raw mode with the alternative terminal, not with the creating a new tui terminal. ([`f1c734f`](https://github.com/byron/prodash/commit/f1c734f92150eff728444042b6c7cbac90989be9))
    - Functionality for Alternate screens ([`dcc92ab`](https://github.com/byron/prodash/commit/dcc92ab05599800940359ac6c37d1e70e15c82f7))
    - Initial terminal implementation, more generic, for tui crossterm ([`e7c07be`](https://github.com/byron/prodash/commit/e7c07be5d93feecc402d0c0793e4a3653c639651))
    - Add everything required to build with stream support ([`2d60cc0`](https://github.com/byron/prodash/commit/2d60cc0149f80197b865312fab84575993ef5df0))
    - Add input stream functions ([`21f4147`](https://github.com/byron/prodash/commit/21f41475e4c07539da49c370f37bd9c73611e94b))
    - Refactor ([`42b8374`](https://github.com/byron/prodash/commit/42b83740c63a928ff3e067fef417d574b1186656))
    - Add Key input transformation ([`bbf2651`](https://github.com/byron/prodash/commit/bbf2651e379b5758d53a889d9fb220c616d2a096))
    - Initial version of crosstermium ([`25c8a98`](https://github.com/byron/prodash/commit/25c8a986ffb96e7c5783478c796c849a258c2ae0))
</details>

## v5.0.0 (2020-07-03)

Support for windows by using Crossbeam by default.
A first low-effort move to the latest version should be to set the dependency to
`default-features = false, features = ["render-tui", "render-tui-termion", "localtime", "log-renderer"]`
to get the same configuration as before.

To try crossbeam, use `with-crossbeam` instead of `render-tui-termion`.

If you have been using the event stream to send your own keys, swap `termion::event::Key` with `prodash::tui::input::Key`.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 16 commits contributed to the release.
 - 2 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Major release: windows support ([`656cb6a`](https://github.com/byron/prodash/commit/656cb6a52625ce5c6bb0e35e84f077b1c2b2243b))
    - Make with-crossbeam the default for maximum compatibility ([`9b8be9e`](https://github.com/byron/prodash/commit/9b8be9e41db9ecf8dd6509e081aa3d26863839f6))
    - Fix makefile check; remove termion from dev dependenices ([`aa5827d`](https://github.com/byron/prodash/commit/aa5827d5988adf931f499b95df092713ab97a2b1))
    - Make runtime into compile time error, nice! ([`7fc5961`](https://github.com/byron/prodash/commit/7fc5961d8a7059011f0c9774e5ec10a6240df672))
    - Refactor; actually try compiling on windows ([`52f2592`](https://github.com/byron/prodash/commit/52f2592461d6289b944d4dee339eb60cec46e0c9))
    - Refactor ([`72fe1bb`](https://github.com/byron/prodash/commit/72fe1bbd8dcbc79424f40f47e487fb1be5f9b8b6))
    - Refactor ([`ababea5`](https://github.com/byron/prodash/commit/ababea51c0bddf82d634fc3d6e0e7d3448d074dc))
    - Fix crossterm input handling ([`8547c8b`](https://github.com/byron/prodash/commit/8547c8bc4489c5934653757250b2735102ea5493))
    - First crossterm input event loop, but looks like keys are not converted properly ([`9e74ada`](https://github.com/byron/prodash/commit/9e74ada3fd730ff90110bad813fffa4f1c86a683))
    - Implement crossterm buffer creation (alternate + raw) ([`eaf904b`](https://github.com/byron/prodash/commit/eaf904bfb1014f94bc0422ef167c671bf0b1794c))
    - Fix accidental import ([`659065d`](https://github.com/byron/prodash/commit/659065d7f44439104dff6c8128c0ed7a4b08aca5))
    - And map crossterm modifiers on a best-effort basis ([`bafb189`](https://github.com/byron/prodash/commit/bafb189d3d200d57edcc9c7936e3ac37fb6640a0))
    - First stab as crossterm key event mapping ([`6650a36`](https://github.com/byron/prodash/commit/6650a368df002c2548b926cc72852541d9f3fd46))
    - Document all tui-renderer toggles and test termion feature toggles ([`27ccdc1`](https://github.com/byron/prodash/commit/27ccdc188d4431a88ea8be0248a9dcc755c9eaa8))
    - Allow graceful failure if no backend is chosen. ([`1f36d96`](https://github.com/byron/prodash/commit/1f36d9644983cea6c465cdc290d6c1f8d3e3b2f0))
    - Put all usage of termion behind a feature flag and unify Key input ([`3a1dc75`](https://github.com/byron/prodash/commit/3a1dc75a0c4ca3f5b888c68de91b420f261837dd))
</details>

## v4.1.0 (2020-07-01)

Allow the TUI to automatically stop if there is no progress to display.

This way, it's easier to use `prodash::tui` for visualizing finite tasks, which originally it wasn't intended for.

Previously, in order to achieve the same, one would have to initialize the TUI with an event stream and send the Event
for shutting down once the task at hand is complete.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump minor version ([`6755f32`](https://github.com/byron/prodash/commit/6755f326f852b1cf9a8ec64d31adc7bbb3fdcaa7))
    - Allow the TUI to automatically stop if there is no progress to display ([`4fb6079`](https://github.com/byron/prodash/commit/4fb607994b72dcf17965b0424f495ef9d0875400))
</details>

## v4.0.5 (2020-07-01)

Fix delayed reset of the terminal.

Previously even after the future was dropped, it seemed like the terminal wasn't reset and the user was required
to explicitly flush stdout to make the changes appear. This is due to the flushing previously happening too early,
that is, before the `terminal` was dropped which emits the respective terminal escape codes at this time.

Now the terminal instance is dropped explicitly right before emitting a flush.
One might argue that the flush should happen in the terminal instance itself, but fixing that is out of scope.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 1 day passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump patch level ([`f8b06a6`](https://github.com/byron/prodash/commit/f8b06a6a298928caaf2a6e6e9b2e3135d0ae443b))
</details>

## v4.0.4 (2020-06-29)

- Simplify `message()` trait bounds

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Simplify traitbounds; bump patch level ([`be4240e`](https://github.com/byron/prodash/commit/be4240e94aacdde26b58d68ce7550c3e1aaa0094))
    - (cargo-release) start next development iteration 4.0.4-alpha.0 ([`929225d`](https://github.com/byron/prodash/commit/929225dee1e2c8936a27d127765019ecbc9ee1ad))
</details>

## v4.0.3 (2020-06-29)

- Remove piper in favor of futures-channel (which was included anyway)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release over the course of 33 calendar days.
 - 42 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Remove async-channel in favor of (anyway imported) futures-channel ([`e4c2501`](https://github.com/byron/prodash/commit/e4c250106c2f1d81a88793c7ee74f74f21b35d68))
    - Prepare next release ([`9879999`](https://github.com/byron/prodash/commit/9879999f66bccad3e4d43173bbaa5cc9a5126417))
    - Update dependencies ([`73c25c2`](https://github.com/byron/prodash/commit/73c25c2ca1d95188543574ab1490961f69d001cf))
    - Optimize include directive with 'cargo diet' ([`2978d2a`](https://github.com/byron/prodash/commit/2978d2a40b5d2a421029f7838515857cbfb45f08))
</details>

## v4.0.2 (2020-05-17)

- Upgrade to latest TUI and TUI-react crates

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Upgrade tui-* crates; bump patch level ([`7b796d1`](https://github.com/byron/prodash/commit/7b796d14cc5d2c9ceb3118e0cbfa3e3abfa86668))
</details>

## v4.0.1 (2020-05-17)

- Reduce theoretical direct dependencies by not using 'futures' crate directly

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump patch level ([`bf4fa4f`](https://github.com/byron/prodash/commit/bf4fa4f2fdea700f1ce254518d6791348dbe9a45))
    - Reduce amount of dependencies by 5 ([`03970db`](https://github.com/byron/prodash/commit/03970db3d56a6ac960e95f6c67b8e4250be86791))
</details>

## v4.0.0 (2020-05-17)

Switch from futures executor to smol.

This actually simplifies some parts of the implementation, while fixing issues along futures not being dropped while they
were on a thread pool. Now, for the example, no threadpool is used anymore.

**Note** that this also means that in order for each frame to be drawn, one would have to invoke `smol::run` in one thread to
activate the reactor which processes the timeout/ticker. Alternatively, one would send `Tick` events through a channel to trigger
a redraw manually.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 12 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Replace futures-timer with smol; major version bump ([`ecb7dc8`](https://github.com/byron/prodash/commit/ecb7dc8e925c4acea08908c5c61e453d553ceec7))
</details>

## v3.6.3 (2020-05-05)

- Fix out-of-bounds access (and panic) due to new and more precise progress bars

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 1 day passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump patch level ([`2803e46`](https://github.com/byron/prodash/commit/2803e4698bb01fea2ff06a67af7b7db5e87da564))
    - A precaution to make out-of-bounds access impossible ([`205a0ef`](https://github.com/byron/prodash/commit/205a0ef418fa5ec645191eecdf5adfe47f0051df))
</details>

## v3.6.2 (2020-05-03)

- More horizontally precise progress bars; progress bars are now have lines between them vertically

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 5 commits contributed to the release over the course of 21 calendar days.
 - 24 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump patch level; fix crash due to integer underflow/overlow ([`af60c44`](https://github.com/byron/prodash/commit/af60c44ad27eab2cc0bdcda7b37b2df1c1176f48))
    - More fine-grained progress bars, both horizontally and vertically ([`d6b7b3f`](https://github.com/byron/prodash/commit/d6b7b3f23215d281b8cecf64423036ddb3ae9081))
    - Add clippy and fmt lints to actions ([`c56825c`](https://github.com/byron/prodash/commit/c56825cb46233289f8e638f3b7b941fcf09e8592))
    - Bye bye travis, it was a good time! ([`d29fe5c`](https://github.com/byron/prodash/commit/d29fe5cbfaec8e492bda9c5a6821c88f5b227b41))
    - Add github actions to compile and test ([`9efa70c`](https://github.com/byron/prodash/commit/9efa70c966446f9393b9495102ea892e4c3a989e))
</details>

## v3.6.1 (2020-04-09)

- Properly respond to state changes even when 'redraw_only_on_state_change' is enabled

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Patch bump; redraw also when the state changes, otherwise TUI feels laggy ([`93fce71`](https://github.com/byron/prodash/commit/93fce7193d4fc926ab0d31c868051c22b7258ece))
</details>

## v3.6.0 (2020-04-09)

- A TUI option to only redraw if the progress actually changed. Useful if the change rate is lower than the frames per second.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump minor ([`f209f60`](https://github.com/byron/prodash/commit/f209f6034492422550d2de24fbcecdee71a370cf))
    - Add capability to redraw only on state change ([`5fa836f`](https://github.com/byron/prodash/commit/5fa836fa8f035e73c0e4ee3174725ef28c2a93a0))
</details>

## v3.5.1 (2020-04-09)

- Don't copy messages if the message pane is hidden, saving time

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 5 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump patch level; copy messages only if the message pane is shown ([`353763b`](https://github.com/byron/prodash/commit/353763bfd588285e1e3770ea9b52a0a1fe179837))
</details>

## v3.5.0 (2020-04-03)

- Cleaner visuals for hierarchical progress items, these won't show lines if there are no direct children with progress

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump minor ([`39fbc16`](https://github.com/byron/prodash/commit/39fbc16a592703bd6d946a5ee5e2d1a48156042d))
    - Don't show lines if these nodes are not actually children with progress ([`058b73a`](https://github.com/byron/prodash/commit/058b73a1f603e1538ba24172ce0715bfc0048b4a))
    - Fix badge ([`97df7da`](https://github.com/byron/prodash/commit/97df7da752b09578aa7adad0d7c2d1866e6570f3))
</details>

## v3.4.1 (2020-04-02)

- Enable localtime support by default

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump minor - enable localtime by default ([`662f186`](https://github.com/byron/prodash/commit/662f186562423de7b955e24ba8432ea70ae6092b))
    - Enable localtime by default - pulling time in isn't all that bad ([`699beba`](https://github.com/byron/prodash/commit/699bebaa9028602cd082f2b85f6531730db8e22b))
    - Update asciinema video ([`0f07b68`](https://github.com/byron/prodash/commit/0f07b68ee60f1c69b3ea64f9d7792114fd5a293b))
</details>

## v3.4.0 (2020-04-02)

- Even nicer tree rendering, along with screen space savings

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 24 commits contributed to the release over the course of 2 calendar days.
 - 2 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump minor ([`bbdc625`](https://github.com/byron/prodash/commit/bbdc6252130c05770b6524222435e951e2ded031))
    - Better orphan display, with dots leading up to the node's level ([`4d69c34`](https://github.com/byron/prodash/commit/4d69c3436ae0e1d5345adeff7ca8cdf575961fa5))
    - Use space much more efficiently in the tree view as well ([`3430a8d`](https://github.com/byron/prodash/commit/3430a8d46e251b166aba70dd078ce914de845dbf))
    - Omit line between tree on left and progress bars to safe space ([`a3e5fe5`](https://github.com/byron/prodash/commit/a3e5fe5d2489028bc764fc0e771707419176a957))
    - Oh my, that was sooooo worth it! ([`31bb906`](https://github.com/byron/prodash/commit/31bb906c643c67d95735b22d0aba0d883425036e))
    - Looks pretty good already, now the tweaking ([`7f16472`](https://github.com/byron/prodash/commit/7f164723dca2723c9e37fd4773820576049f4f6d))
    - Make orphan nodes work ([`497fbce`](https://github.com/byron/prodash/commit/497fbcee0d7ceda90eafbf3800d8ca68f6da0c3a))
    - All tests green for the first time! ([`edf2024`](https://github.com/byron/prodash/commit/edf20248be59998553a47a70a764a64fb69b482c))
    - Maybe a tiny step closer, definitely better/fixed index handling ([`42b31da`](https://github.com/byron/prodash/commit/42b31da9262c6842243a998bb98960b5786b9b1e))
    - Before it all goes down the drain - only one thing to fix - let's checkpoint it ([`83a89ee`](https://github.com/byron/prodash/commit/83a89ee194db870bf267bf12f6ee73daaff1502d))
    - I think the problem is that I try to squash a hiarchy level. Let's not do that ([`3c77eab`](https://github.com/byron/prodash/commit/3c77eab4152683babc1921022397ece103e1a790))
    - Is this ever going to work? ([`8236d55`](https://github.com/byron/prodash/commit/8236d557eb03179e382107bc3cdb40cb1c488aba))
    - A little closer, but needs way more tests :D ([`14876ee`](https://github.com/byron/prodash/commit/14876ee3fd5f236da9d8811096400f1728a31e3f))
    - Refactor ([`8f6061f`](https://github.com/byron/prodash/commit/8f6061ffa75d18da2ac51d701700a90adc3b71b6))
    - Better, but only works correctly for a single level - more nesting required  Please enter the commit message for your changes. Lines starting ([`4083c4e`](https://github.com/byron/prodash/commit/4083c4e77fce5b12adb837438fb43ba7de98b642))
    - Now we are actually closing in for this to work…OMG! ([`e9d268f`](https://github.com/byron/prodash/commit/e9d268f6ef670ec3c4428166d22e30c74d5eaab8))
    - A little better ([`15a2b02`](https://github.com/byron/prodash/commit/15a2b02110135326598699733ba127a905aade2b))
    - Add first tests for adjacency computation - it's not exactly working out of the box *:D ([`4e033b4`](https://github.com/byron/prodash/commit/4e033b4f20cca4d23ac23a3a42fd13ed50f332d5))
    - Add everything needed to make use of the new system, and it's totally not working :D ([`40690b6`](https://github.com/byron/prodash/commit/40690b66f70ad96feddf92b6377838abeec76dbd))
    - This might even work…one day :D ([`4652654`](https://github.com/byron/prodash/commit/46526549cb1392dc37552effa0837f146116d69c))
    - Getting there, slowly, need refactor ([`46d015c`](https://github.com/byron/prodash/commit/46d015cbc3de26f54185f78fa0af8f90054f08bf))
    - First sketch on getting an adjecency map by searching a sorted list of entries ([`0838165`](https://github.com/byron/prodash/commit/08381650f157f9430d7155d3543cb0bdfeab4864))
    - Draw progress lines without ellipsis; make clearer that this happens ([`678fbaa`](https://github.com/byron/prodash/commit/678fbaa085ea0680f7f2212d27e6a577f65091b4))
    - Improve tree drawing further ([`adc49c6`](https://github.com/byron/prodash/commit/adc49c624b21b6e200c4d00e29406c1262fd0c61))
</details>

## v3.3.0 (2020-03-31)

- Much nicer task tree visualization

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 6 commits contributed to the release over the course of 1 calendar day.
 - 2 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump minor ([`cf9ddb8`](https://github.com/byron/prodash/commit/cf9ddb89414a844120b60b2a5d004a411f73cf49))
    - Much nicer display of trees, now also on the level of progress bars ([`706d658`](https://github.com/byron/prodash/commit/706d6589b23b0df3f634017f53189074997d38f5))
    - Refactor ([`536b4c8`](https://github.com/byron/prodash/commit/536b4c878206f9c6e6fcf443bae701af628cb0d4))
    - A nicer way to draw the progress - show the tree even in the progress bars ([`840bb3a`](https://github.com/byron/prodash/commit/840bb3a58099b13086fe325631d8858f6546252b))
    - Much better tree display, even though the code doing it is a bit wonky :D ([`833bb98`](https://github.com/byron/prodash/commit/833bb982eb76ead49e86f021537a66a2af404b4b))
    - Use all functionality that now is available in tui-react ([`f5b1ee1`](https://github.com/byron/prodash/commit/f5b1ee109be8000bdae1dd036d82846ceaeb905d))
</details>

## v3.2.0 (2020-03-28)

- Application can control if the GUI will respond to interrupt requests
 

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 2 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump minor; dont' discard previous value of 'interrupt requested' ([`be1ce75`](https://github.com/byron/prodash/commit/be1ce7526ba6d8abf85371f6685664390835ba01))
    - Visualize the interrupt-requested state, while providing useful information ([`273a1a6`](https://github.com/byron/prodash/commit/273a1a62bf29a6562c246079c3deeb182cdf4ebe))
</details>

## v3.1.1 (2020-03-25)

- Bugfix (really): Finally delayed column resizing works correctly.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump patch - fix column sizing for good… ([`5029823`](https://github.com/byron/prodash/commit/50298236bfd32de329862540bdd00e1e1fae9fec))
</details>

## v3.1.0 (2020-03-25)

- Tree::halted(…) indicates interruptable tasks without progress. Tree::blocked(…) means non-interruptable without progress.
 

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Support for 'halted' state to indicate interruptible tasks; bump minor ([`b5ef271`](https://github.com/byron/prodash/commit/b5ef2716b52ecf145c0aff458a05f44b0792ce7a))
</details>

## v3.0.2 (2020-03-25)

- Bugfix: Allow column-width computation to recover from becoming 0
 

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Allow column width computation to recover from 0; bump patch ([`856b7ee`](https://github.com/byron/prodash/commit/856b7ee3206d60cb9038f06822515fba0e23688d))
</details>

## v3.0.1 (2020-03-25)

<csr-id-82baf266045d44ba31aad4e570c687d7c51d0df7/>

- Bugfix: Don't allow values of 0 for when to recompute task column widths
 

### Other

 - <csr-id-82baf266045d44ba31aad4e570c687d7c51d0df7/> assure we never try to do 'x % 0' :D

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Assure we never try to do 'x % 0' :D ([`82baf26`](https://github.com/byron/prodash/commit/82baf266045d44ba31aad4e570c687d7c51d0df7))
</details>

## v3.0.0 (2020-03-25)

- New TUI option to delay computation of column width for stability with rapidly changing tasks
 

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump version to 3.0.0 ([`bc46acc`](https://github.com/byron/prodash/commit/bc46accd27868ad3bbb21321bee0af114d185fce))
    - Allow to control column width recomputation ([`d4dc966`](https://github.com/byron/prodash/commit/d4dc966dc5ae1d907f7b830a03477f84baead855))
    - Support to compute column size only every so many ticks… ([`98e0f63`](https://github.com/byron/prodash/commit/98e0f630ffe858b989e7c973de1556a4477a0c8f))
    - Example dashboard: Make sure ticker for context does not outpace FPS ([`c25217b`](https://github.com/byron/prodash/commit/c25217b54fd23d2c450211763a16270f39c3efdb))
</details>

## v2.1.0 (2020-03-24)

- Optional cargo feature "localtime" shows all times in the local timezone

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 1 day passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Add cargo feature 'localtime' to show all dates in localtime ([`a87f8cd`](https://github.com/byron/prodash/commit/a87f8cd0e9b9ea70a4a50b954ec3aa3160f2b3ab))
    - Make chrono dependency available via re-exports ([`a5d4423`](https://github.com/byron/prodash/commit/a5d44233c56a9a7e05050e95c1aeaf591546bdcb))
</details>

## v2.0.1 (2020-03-23)

- fix integer underflow with graphemes that report width of 0

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release over the course of 1 calendar day.
 - 15 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Fix integer underflow with graphemes of width 0 ([`d9808cd`](https://github.com/byron/prodash/commit/d9808cd518b80fb4032a9176f1b1ec4dc5a5fdef))
    - Transfer to new github location ([`6fa2e8d`](https://github.com/byron/prodash/commit/6fa2e8dc1b795c5614882e95e184810f7f1e466e))
</details>

## v2.0.0 (2020-03-07)

* BREAKING: `progress.blocked(eta)` now takes a statically known reason for the blocked state `progress.blocked(reason, eta)`. This is
  useful to provide more context.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Reasons for block states; major version bump ([`74abaeb`](https://github.com/byron/prodash/commit/74abaeb4486a3a7b6889c3ed99244ab2c1b0bbf7))
</details>

## v1.2.0 (2020-03-07)

* Support for eta messages in blocked unbounded tasks

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 4 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - ETA messages in blocked unbounded tasks ([`d42c0d5`](https://github.com/byron/prodash/commit/d42c0d52f803595a48594a91b3278ce57ff8b95b))
</details>

## v1.1.6 (2020-03-02)

* improve API symmetry by providing a `Tree::name()` to accompany `Tree::set_name(…)`

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release over the course of 5 calendar days.
 - 8 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Improve API symmetry ([`759f1f4`](https://github.com/byron/prodash/commit/759f1f4164f21804336f2c5677322b59c4218f7a))
    - Put changelog into lib.rs, where it already was :) ([`dfc810b`](https://github.com/byron/prodash/commit/dfc810b7b012e536353ceeed61f2ca7ed935930d))
</details>

## v1.1.5 (2020-02-23)

* Flush stdout when the TUI stopped running. That way, the alternate/original screen will be shown right away.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - V1.1.5 - flush stdout right away ([`8734668`](https://github.com/byron/prodash/commit/87346682d74951ed332a2533c00232095cc1d40b))
</details>

## v1.1.4 (2020-02-23)

* Don't pretend to use &str if in fact an owned string is required. This caused unnecessary clones for those who pass owned strings.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Version bump ([`15d9681`](https://github.com/byron/prodash/commit/15d9681acd734c8518a541f621ccc69c24d9a0f3))
    - Cargo clippy ([`3adba17`](https://github.com/byron/prodash/commit/3adba17c71b8e98634cf651219ce247b2182174e))
    - Don't pretend we only need str ([`d05158b`](https://github.com/byron/prodash/commit/d05158b135bab195dc8a34a573cee120079146b2))
</details>

## v1.1.3 (2020-02-23)

* hide cursor or a nicer visual experience

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 3 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Hide cursor, bump version ([`ecf5cc4`](https://github.com/byron/prodash/commit/ecf5cc4a55536d36290b8bea1f816d3d29bb5468))
    - Run the few doc tests we have ([`9b28cc0`](https://github.com/byron/prodash/commit/9b28cc0fce171bbadf736d25d8aec1614f24caf7))
    - (cargo-release) start next development iteration 1.1.3-alpha.0 ([`cae0e02`](https://github.com/byron/prodash/commit/cae0e024d017d84962dc459e200d361c372f9c89))
</details>

## v1.1.2 (2020-02-22)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 5 commits contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Show travis badge ([`65b48f1`](https://github.com/byron/prodash/commit/65b48f1270310453e7b03363ddfdad5551aa6900))
    - Bump version for new crate meta-data ([`c557b6b`](https://github.com/byron/prodash/commit/c557b6b05206822e2b01feb079d2b8745b408019))
    - Travis support ([`507cc20`](https://github.com/byron/prodash/commit/507cc207742b536a0bd92a4357f6cfc1b5b2126b))
    - Adjust repository link ([`d75d91f`](https://github.com/byron/prodash/commit/d75d91fe16e32747bb6400cbe3b3e63c4399f123))
    - Initial commit, as copied from cli ([`708901b`](https://github.com/byron/prodash/commit/708901b796110bd49bcac6bc1ef1daa7b1931720))
</details>

## v1.1.0

* fix toggles - previously prodash, withoug tui, would always build humantime and unicode width
* add support for logging as user interface

## v0.7.0 (2021-05-02)

## v0.6.0 (2021-01-04)

## v0.5.0 (2020-11-15)

## v0.4.0 (2020-09-28)

## v0.3.2 (2020-09-14)

## v0.3.1 (2020-09-13)

## v0.3.0 (2020-07-22)

## v0.2.0 (2020-07-11)

## v0.1.4 (2020-07-06)

## v0.1.3 (2020-07-06)

## v0.1.2 (2020-07-06)

## v0.1.1 (2020-07-05)

