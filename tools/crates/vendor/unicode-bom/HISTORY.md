# Change log

## 2.0.3

### Other changes

* code: drop unnecessary `unsafe impl` for `Send` and `Sync` (21d7837889515dbc15a381ca00b7a55711e38828)

## 2.0.2

### Other changes

* docs: fix version string in readme (8f49df5c92c176a7bd3e296992eda4fdef8b5c96)

## 2.0.1

### Bug fixes

* docs: fix misleasing typo in doc comment (dfc5982cf7238fc8f0b00ad7958ea89092062a84)

## 2.0.0

### Breaking changes

* api: implement AsRef<[u8]> for Bom (45c0e3ed98a555ee31fcdfb1740e64b7e30b9967)

### Other changes

* project: switch dev environment to stable (a536298f7c1f138787ea05764acfd62a24ac9745)

## 1.1.4

### Refactorings

* logic: reinstate macros for tail comparison (014ad76)
* tests: prefer functions to macros for assertions (decdd93)

### Other changes

* code: move private functions to bottom of module (cd5d6ec)

## 1.1.3

### Refactorings

* logic: prefer functions to macros for comparisons (b16bdf4)

## 1.1.2

### Other changes

* code: cargo fmt (a8573f0)

## 1.1.1

### Refactorings

* logic: cleaner compare_tail! macro syntax (3a6ca2d)
* tests: rename assert_slice! macro to assert_bom! (ec71494)

### Other changes

* tests: extra assertions to ensure trailing bytes are ignored (49e9005)
* docs: fix weird version replacement insanity in change log (c346ce3)

## 1.1.0

### New features

* api: implement Bom::len() to return the size in bytes (c5d5810)

### Other changes

* docs: add a doc comment for the FromStr Err type (5406fbd)

## 1.0.0

### Breaking changes

* api: prefer FromStr to From<&str> for parsing from named files (6e76812)

### Bug fixes

* ci: fix writes to public/ in pages builds (da9e7cb)

### Other changes

* docs: link to API docs from readme (aca453d)
* docs: add content links to readme (ecfa529)

## 0.1.2

### Other changes

* crate: fix gitlab build status badge (16f5a9a)

## 0.1.1

### Bug fixes

* crate: update docs link to point to gitlab instead of docs.rs (5085a1a)
* docs: fix license badge URL (ef2f7ef)
* docs: remove stale old reference to Path in readme (f5f9bc9)

### Other changes

* ci: build docs in CI (83ba4b3)
* ci: build on stable in CI (5e57fa4)
* crate: add toolchain file (bf10c10)
* crate: link to docs (fc7a19e)

## 0.1.0

### New features

* docs: write some basic documentation (e472f2a)
* types: implement From<&Path> for Bom (a63deca)
* types: implement From<&mut File> for Bom (73924e1)
* types: implement From<&[u8]> for Bom (0007285)
* types: implement a basic Bom enum (b30af8b)

### Refactorings

* api: accept a plain &str as a file path (1ac8883)

### Other changes

* tests: simplify some test code (e832f1a)
* code: silence clippy warnings (b3d3bd0)
* ci: add ci config (99dd906)
* repo: add git ignore file (2bad0b2)

## 0.0.0

* chore: initial commit

