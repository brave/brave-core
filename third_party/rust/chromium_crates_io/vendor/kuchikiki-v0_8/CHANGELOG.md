# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## v0.8.2 (2023-05-15)

New release from the Brave Browser maintainers, renamed to `kuchikiki`.
It should be semver-compatible with the old [kuchiki](https://github.com/kuchiki-rs/kuchiki) crate.

This includes accumulated fixes from the former upstream since 0.8.1.

### Changes

 - <csr-id-4a14e45ee73d6cbf7b7f242bd72bdfc3b0e510bd/> Remove unnecessary to_string
 - <csr-id-d2118f6f63ea1f82317b886990710f2b9059d147/> Mark our fork as maintained.
   We're at least maintaining our fork for now. Link to upstream's
   documentation since the default branch API hasn't changed.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 33 commits contributed to the release over the course of 732 calendar days.
 - 1013 days passed between releases.
 - 2 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Add a changelog (f250608)
    - Update version number of 0.8.2 (68400f0)
    - Merge pull request #13 from brave/kuchikiki (14392f9)
    - Update doc comment for the crate name change (7dc2624)
    - Remove obsolete docs website. (4f4fb63)
    - Rename crate to kuchikiki (5a615a6)
    - Merge pull request #10 from brave/clippy (0a5973e)
    - Remove unnecessary to_string (4a14e45)
    - Use Iterator::find (63179ba)
    - Merge pull request #11 from brave/wknapik-lint-permissions (78136d1)
    - Added checks:write permission to the lint action (6286dca)
    - Merge pull request #5 from brave/ci (def867c)
    - Fix last format issue (aa5bc00)
    - Github actions: add lint job (3d2fe32)
    - Add Security policy and an audit ci job (c31cee6)
    - Replace travis ci with minimal github action. (dc64a57)
    - Merge pull request #8 from brave/clippy (c601610)
    - Merge pull request #6 from brave/fmt (962ea65)
    - Remove unnecessary double-ref pattern (45f12b9)
    - Remove unnecessary deref (bde468b)
    - Merge pull request #7 from brave/codeowners (b99303e)
    - Set codeowners to the Brave rust review team (fb7278c)
    - Apply standard formatting (befa32c)
    - Merge pull request #4 from brave/update (3b391b5)
    - Mark our fork as maintained. (d2118f6)
    - Archived (f92e4c0)
    - Update URL (22004a8)
    - Upgrade html5ever to v0.26 (d218ccb)
    - Add missing `!` to example's doctype (7ee2c78)
    - Replace tempdir with tempfile, as tempdir is deprecated (d510aa8)
    - Replace BTreeMap with IndexMap in attributes.rs, in order to preserve attribute order (a81f510)
    - Update Cargo.toml to kuchiki-rs/kuchiki (c6ccc6f)
    - Add MIT license file (c52fe05)
</details>

## v0.8.1 (2020-08-05)

Last release under the original `kuchiki` name.

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 7 commits contributed to the release over the course of 170 calendar days.
 - 212 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Update kuchiki version (f652e38)
    - Remove std::ascii:AsciiExt, use inherent method instead (f09b2d4)
    - Parse_and_serialize_fragment test (3a54eee)
    - Defined parse_fragment() and parse_fragment_with_options() functions (d46961e)
    - Fix clippy warnings (596ecac)
    - Expose Sink type (300a20a)
    - Previous -> next (b4ee7dd)
</details>

## v0.8.0 (2020-01-05)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 5 commits contributed to the release over the course of 136 calendar days.
 - 245 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Update to 0.8.0. (6211094)
    - Upgrade html5ever, cssparser, and selectors. (8aa1874)
    - Upgrade to 2018 edition. (57ee692)
    - Remove deprecated try! use. (d1d2daf)
    - Use 'dyn' since trait objects without an explicit 'dyn' are deprecated (fc6afc0)
</details>

## v0.7.3 (2019-05-05)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 7 commits contributed to the release over the course of 246 calendar days.
 - 254 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - 0.7.3 (ec56dbf)
    - Remove obsolete trait import (b785a7c)
    - Update dependencies (c0de72f)
    - Rustfmt (f8cd160)
    - Use std::cell::Cell + extension traits instead of a custom MoveCell (616690f)
    - Add a test to demonstrate pre-compiled selectors (66d8e2b)
    - Make NodeDataRef support == comparison (3b59170)
</details>

## v0.7.1 (2018-08-23)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 219 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Update to cssparser 0.24 and selectors 0.20 (3196c7d)
</details>

## v0.7.0 (2018-01-15)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 6 commits contributed to the release over the course of 67 calendar days.
 - 110 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Change Attributes to a BTreeMap (29721b3)
    - Optimize attribute selector matching (c64ba44)
    - Move namespace prefix of attributes out of the hashmap key (e173d3d)
    - Update html5ever (7387037)
    - Update selectors and cssparser (30466ea)
    - Updated html5ever to 0.21. (1544f40)
</details>

## v0.6.0 (2017-09-27)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 15 commits contributed to the release over the course of 128 calendar days.
 - 156 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - V0.6.0 (breaking update of public dependency html5ever) (05e7099)
    - Implement TreeSink::append_based_on_parent_node (3df7ab7)
    - Update html5ever to 0.20.0 (1be51d4)
    - Replaced nth(0) with next(). (48f33a6)
    - Added NodeRef::select_first. (d145c39)
    - Update kuchiki version (8bf029a)
    - Update html5ever version (9eb7003)
    - Fix Travis-CI for removal of the 'unstable' Cargo feature (3b94d26)
    - (Hopefully) fix Travis-CI builds for Hyper removal. (dce9899)
    - Remove the `cargo doc` CI command (ad69f67)
    - Update selectors to 0.18, cssparser to 0.13 (487928d)
    - Udpate selectors to 0.17 (2e2dde1)
    - Update html5ever to 0.17 (00a4288)
    - Update to html5ever 0.16 (d28f83e)
    - Remove Hyper support (3c38d02)
</details>

## v0.4.3 (2017-04-24)

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
    - Implement Debug and Display for Selector and Selectors (9b8eb50)
    - Update to selectors 0.15 (11660e0)
</details>

## v0.4.2 (2017-04-23)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release over the course of 78 calendar days.
 - 99 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Expose Selector and Specificity (ebe066f)
    - Updated html5ever to 0.13.1 (5c0118f)
    - Re-export the custom parse function (27758d9)
    - Rc_counts is stable in Rust 1.15, remove crates.io rc dependency. (6cd4d95)
</details>

## v0.4.1 (2017-01-12)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 67 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bump version (3621cea)
    - Allow Hyper 0.10 (49fd968)
</details>

## v0.4.0 (2016-11-06)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release over the course of 58 calendar days.
 - 69 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Bumped version to 0.4.0 because breaking changes. (bd17701)
    - Updated html5ever to 0.9.0. (25cf952)
    - Updated cssparser & selectors. (c970cc8)
    - Move docs to docs.rs (9ee0e76)
</details>

## v0.3.4 (2016-08-29)

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
    - Update to selectors 0.12 (47e61a0)
</details>

## v0.3.3 (2016-08-16)

<csr-id-363573b511c2f99a805dd77373c075923b670af6/>

### Other

 - <csr-id-363573b511c2f99a805dd77373c075923b670af6/> :set was unsound

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 6 commits contributed to the release over the course of 123 calendar days.
 - 144 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 1 unique issue was worked on: #27

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **#27**
    - Update rust-selectors (fa51de5)
 * **Uncategorized**
    - V0.3.3 (a82f4bc)
    - Merge pull request #26 from untitaker/update-hyper (43b7093)
    - (chore) Update hyper (5508a64)
    - :set was unsound (363573b)
    - Update tests for html5ever 0.5.4 (c11fb34)
</details>

## v0.3.2 (2016-03-25)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release over the course of 10 calendar days.
 - 27 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - V0.3.2 (e8cb559)
    - Merge pull request #22 from untitaker/rustup (ea021e9)
    - Update to hyper 0.8 (0cad2ce)
    - Update rust-selectors (dc7295f)
</details>

## v0.3.1 (2016-02-26)

<csr-id-14ce4983faa8d20f46daf0b975036b0c9ef04df5/>

### Other

 - <csr-id-14ce4983faa8d20f46daf0b975036b0c9ef04df5/> No xml yet

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 6 commits contributed to the release over the course of 22 calendar days.
 - 31 days passed between releases.
 - 1 commit was understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - From_http() can take an existing Response now. (ae88c8a)
    - No markdown footnotes for github (2b6bb80)
    - No xml yet (14ce498)
    - Expand docs on optional features. (2e6d261)
    - Fix the hyper example. (7398a60)
    - Add a test for parsing from bytes. (e34d4cb)
</details>

## v0.3.0 (2016-01-26)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 4 commits contributed to the release.
 - 7 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Test Hyper support on Travis (890f4f5)
    - Merge pull request #19 from SimonSapin/tendrilsink (e77cf98)
    - Move Hyper support back from html5ever. (04f846d)
    - Upgrade html5ever to a TendrilSink based API. (d3c856c)
</details>

## v0.2.1 (2016-01-19)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Fix add_attrs_if_missing to only add if missing. (13a914d)
</details>

## v0.2.0 (2016-01-19)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 1 commit contributed to the release.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Add convenience methods for accessing attributes. (29d69a2)
</details>

## v0.1.2 (2016-01-18)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 2 commits contributed to the release.
 - 55 days passed between releases.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Add basic support for Hyper. (efcf52d)
    - Upgrade to rustc 1.7.0-nightly (d0bac3f14 2016-01-18) (010f483)
</details>

## v0.1.1 (2015-11-24)

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
    - Upgrade to string-cache 0.2 (b5e62f6)
</details>

## v0.1.0 (2015-10-28)

### Commit Statistics

<csr-read-only-do-not-edit/>

 - 96 commits contributed to the release over the course of 205 calendar days.
 - 0 commits were understood as [conventional](https://www.conventionalcommits.org).
 - 0 issues like '(#ID)' were seen in commit messages

### Commit Details

<csr-read-only-do-not-edit/>

<details><summary>view details</summary>

 * **Uncategorized**
    - Use rust-selectors from crates.io (cb796ed)
    - Merge pull request #14 from marvelm/master (2f1b648)
    - Implement tree::Element::get_target_state (3f1df37)
    - Add support for <template> (94c64f2)
    - Merge pull request #12 from Ygg01/example (8658057)
    - Add example of Kuchiki parsing a page (e4e1b49)
    - Upgrade to rustc 1.4.0-nightly (8f1b0aa32 2015-08-21) (680c2cb)
    - Update selectors. (8364e52)
    - Merge pull request #10 from ucarion/ucarion-example (1224b5b)
    - Add .text_contents() -> String (765dac5)
    - Add a very simple example. (d9de00c)
    - Inline (almost) all the methods. (7c82a9f)
    - Don’t implement PartialEq for Node, only NodeRef. (2a3bc5b)
    - Remove unused movecell dependency. (a41a5d7)
    - Have Travis-CI test on stable Rust. (afef631)
    - Add a doc link (07cf134)
    - Document ALL THE THINGS! (821d858)
    - Make Node::data private, with a read-only accessor. (3b6f76e)
    - Implement FromStr for Selectors (327b822)
    - Make Selectors::filter take &self instead of self. (954cc90)
    - Move the Select iterator to the select module. (2dc0aa9)
    - Move NodeDataRef into a module. (843d2e8)
    - Generalize OptionCell back to MoveCell. (e066e67)
    - Move iterators to a module. (0e68a9d)
    - Run on stable Rust (3e7f222)
    - Return io::Error rather than panic when reading. (4b70cae)
    - Add `Html::from_stream` which takes any `std::io::Read` stream of bytes. (0e87e29)
    - Have Travis-CI upload rustdoc output to GitHub Pages. (0f89e18)
    - Typo fixes (1e9f6d0)
    - Add a comment about tree consistency. (ed416a6)
    - Add `impl Drop for Node` to avoid overflowing the stack. (c05dec7)
    - Add CellOption::set, like replace without using the return value. (58ccd6f)
    - Use a specialization of MoveCell (9c7a3bd)
    - Merge pull request #9 from mitaa/debug (a7f547c)
    - Implement Debug for NodeDataRef (e2b74af)
    - :root does not match elements without a parent. (373c89e)
    - Upgrade to selectors 9bd01e1d6e67c346b6a58f5c254b14f59dc1ed07 (b4429ec)
    - Add inclusive variants of next/previous_siblings, descedants, and traverse. (53111fa)
    - Add elements, text_nodes, comments, and select iterator adaptors. (492c2b4)
    - Upgrade to rust-selectors a6cf1fba8f31960254aa62434ab8aeee13aff080 (0891a2c)
    - Upgrade rust-selectors. (fbf75ce)
    - Update rust-selectors (86239a5)
    - Merge pull request #7 from Ygg01/write (6fff41e)
    - Add `serialize_to_file` method to NodeRef (b4c52d8)
    - Minor whitespace fix (2d5451b)
    - Update to Tendril-based html5ever (0b388f9)
    - Have NodeRef::serialize return I/O errors. (e56cb51)
    - Serializing a Document node itself is fine. (464a621)
    - Add a NodeRef::serialize method (1db3d0e)
    - Call `into_*` instead of `as_*` methods that consume a `Rc<_>`. (16e6053)
    - Make the parse error handler a boxed closure rather than a type parameter. (2263fa2)
    - Merge pull request #6 from SimonSapin/rc (303e323)
    - Upgrade to rustc 1.2.0-nightly (2f5683913 2015-06-18) (ff89fab)
    - Upgrade rust-selectors (857ae49)
    - Update rust-selectors (e2ff556)
    - Switch to reference counting. (0387dde)
    - Use html5ever and string-cache from crates.io (af94a15)
    - Merge pull request #5 from Ygg01/text (a7c3475)
    - Add tests for access and modification via Iterator (1667312)
    - Add text_iter method, which returns FilterMap (cb437c6)
    - Rename FilterNodes to SelectNodes (cf5c9e8)
    - Merge pull request #4 from BenoitZugmeyer/to_string_display_node (eb69983)
    - Include the node itself when serializing with to_string (fe1d3b2)
    - Merge pull request #3 from BenoitZugmeyer/fix_node_select (812312a)
    - Simple non-regression test for selecting multiple nodes (569b6b9)
    - Actually iterate over each node with Node::select() (30bc538)
    - Merge pull request #2 from Ygg01/new_api (3afa058)
    - Fix issues in code (9aa2504)
    - Add tests for parse_file (f2b76ee)
    - Removed unnecessary visibility from IgnoreParseErrors (3269b74)
    - Add css method to Node (596430d)
    - Add ToString representation to Node (ec0457a)
    - Add convenience methods for Html struct (38b6a8d)
    - Minor whitespace fixes (ec63f36)
    - Update test for html5ever fix. (0310688)
    - Implement Debug/Clone/Copy on things. (343846b)
    - Implement DoubleEndedIterator for Descendants (3bb97c6)
    - Deduplicate iterator implementations. (44964fd)
    - Simplify iterators, e.g. implement DoubleEndedIterator rather than separate ReverseChildren. (24ea5b0)
    - Have Node implement Eq, with pointer comparaison. (fa06528)
    - Nicer APIs (c1176e3)
    - Remove comment from arena-tree that doesn’t apply. (9719171)
    - More interior-er mutability. (d10c5d5)
    - Link fields should be private after all, to keep them consistent. (113ce7f)
    - Add CSS Selector matching. (1060fcf)
    - Add a parsing + serialization test. (e31bc63)
    - Add serialization. (3a58494)
    - Copy arena_tree in-crate and de-genericize it. Less type-level indirection. (c472607)
    - Move to modules and add a parse() function. (c006862)
    - Have Travis test on Nightly. (1cbb9cb)
    - Switch from RC-based to arena-based tree. (fb4aadf)
    - Test on Travis-CI (1b77b46)
    - Kanji (696f834)
    - Metadata (69ca980)
    - Add html5ever tree sink (c8c5e6b)
    - Empty kuchiki library. (73583ad)
</details>

