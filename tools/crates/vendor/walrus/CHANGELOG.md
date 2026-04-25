# `walrus` Change Log

--------------------------------------------------------------------------------

## Unreleased

Released YYYY-MM-DD.

### Added

* TODO (or remove section if none)

### Changed

* TODO (or remove section if none)

### Deprecated

* TODO (or remove section if none)

### Removed

* TODO (or remove section if none)

### Fixed

* TODO (or remove section if none)

### Security

* TODO (or remove section if none)

--------------------------------------------------------------------------------

## 0.15.0

Released 2020-02-03.

### Added

* Added support for typed `select` instructions.

--------------------------------------------------------------------------------

## 0.14.0

--------------------------------------------------------------------------------

## 0.13.0

--------------------------------------------------------------------------------

## 0.12.0

Released 2019-09-10.

### Added

* Added support for multi-value Wasm!

* Added `ModuleExports::get_exported_{func, table, memory, global}` helper
  functions to get an export by the id of the thing that it is exporting (if
  any).

* Added fuzz testing with libFuzzer and `cargo fuzz`.

### Changed

* No longer using the "derive" feature from `failure`, which should result in
  slimmer dependency graphs and faster builds.

* `Module::emit_wasm` is no longer fallible. It never actually did ever return
  an `Err` and now the type signature reflects that.

--------------------------------------------------------------------------------

## 0.11.0

Released 2019-08-13.

### Added

* `walrus::Module::write_graphviz_dot`: You can now render a whole Wasm module
  as a GraphViz Dot file (previously you could only do one function at a time)
  and it will also show the relationships between all the various Wasm
  structures (previously it only showed the relationships between instructions).

### Changed

* The intermediate representation for instructions (`walrus::ir::*`) has been
  overhauled. `Expr` has been renamed to `Instr`, and an operator no longer
  points to its operands as nested children in the AST. Instead of representing
  every instruction as a node in an AST, now there is a tree of instruction
  sequences. An instruction sequence is a vector of `Instr`s that relate to each
  other implicitly via their effect on the stack. A nested `block ... end`,
  `loop ... end`, or `if ... else ... end` form new nested instruction
  sequences.

* The `Visitor` and `VisitorMut` traits and traversing the IR has also been
  overhauled:

    * Visitors are no longer recursive, and should never recursively call
      `self.visit_foo()` from inside `self.visit_bar()`. Not in the default
      provided trait methods and not in any user-written overrides of those
      trait methods.

    * There are now *traversal functions* which take a visitor, a
      `LocalFunction`, and a start `InstrSeqId`, and then perform some kind of
      traversal over the function's IR from the given start sequence. These
      traversal functions are *not* recursive, and are implemented with explicit
      work lists and while loops. This avoids blowing the stack on deeply nested
      Wasm inputs. Although we can still OOM, we leave fixing that to future
      PRs. Right now there are only two traversals, because that is all we've
      needed so far: an in-order DFS for immutable visitors (needs to be
      in-order so we can encode instructions in the right order) and a pre-order
      DFS for mutable visitors (pre-order is the easiest traversal to implement
      iteratively). We can add more traversals as we need them.

### Removed

* The `Dot` trait has been made internal. Use the new
  `walrus::Module::write_graphviz_dot` method instead.

* The `Visit` trait is no longer exported in the public API, and has been made
  internal. Use the new traversal functions instead (`walrus::ir::dfs_in_order`
  and `walrus::ir::dfs_pre_order_mut`).

--------------------------------------------------------------------------------

## 0.10.0

--------------------------------------------------------------------------------

## 0.9.0

--------------------------------------------------------------------------------

## 0.8.0

Released 2019-06-05.

### Added

* Added `ModuleExports::iter_mut` for iterating over exclusive references to a
  module's exports.

* Added a `ModuleConfig::on_parse` hook, which has access to a map from indices
  in the original Wasm binary to the newly assigned walrus IDs. This is a good
  time to parse custom sections that reference functions or types or whatever by
  index.

* The `TableKind` enum now has various `unwrap_*` helpers to get a particular
  variant's inner data or else panic.

* Added `ModuleFunctions::by_name` to get a function ID by function name.

### Changed

* The `CustomSection::data` trait method now has a new parameter: a map from
  walrus IDs to their indices in the new wasm binary we are emitting. This is
  useful for custom sections that reference functions or types or whatever by
  index.

--------------------------------------------------------------------------------

## 0.7.0

Released 2019-05-17.

### Added

* Added the `walrus::ModuleCustomSections` API for working with arbitrary custom
  sections, including and especially custom sections that `walrus` itself has no
  special knowledge of. This is exposed as the `customs` field of a
  `walrus::Module`.

* Added the `Module::with_config` constructor method to create a default, empty
  module that uses the given configuration.

### Removed

* The `walrus::Module::custom` vector of raw custom modules has been removed and
  is superceded by the new `walrus::ModuleCustomSections` API. If you were using
  this and the old `CustomSection` type, switch to use the `RawCustomSection`
  type with `ModuleCustomSections`.

--------------------------------------------------------------------------------

## 0.6.0

Released 2019-05-02.

### Added

* `ModuleConfig::parse_file` and `Module::parse_file_with_config` helper
  functions to easily parse a Wasm file from disk with a given configuration.

### Changed

* `ModuleConfig::parse` takes `&self` instead of `&mut self` now. This was just
  an oversight / copy-past error before.

--------------------------------------------------------------------------------

## 0.5.0

--------------------------------------------------------------------------------

## 0.4.0

--------------------------------------------------------------------------------

## 0.3.0

Released 2019-02-19.

### Added

* Added support for the [reference
  types](https://github.com/WebAssembly/reference-types/blob/master/proposals/reference-types/Overview.md)
  wasm proposal. [#50](https://github.com/rustwasm/walrus/pull/50)
* Can finish a `FunctionBuilder` with the relevant `&mut` parts of a module,
  rather than a whole `&mut Module`. This is useful when some parts of the
  module are mutably borrowed
  elsewhere. [#56](https://github.com/rustwasm/walrus/pull/56)
* Can get a `FunctionBuilder` from an existing `LocalFunction` so you can build
  new expressions for the
  function. [#54](https://github.com/rustwasm/walrus/pull/54)
* Added the ability to delete functions, imports, exports, etc. Usually it is
  easier to just let the builtin GCing emit only the necessary bits of the wasm
  binary, but manually deleting will remove the item from iterators over the
  module's parts. If you delete a thing, you are responsible for ensuring that
  nothing else is referencing it (eg there are no remaining calls to a function
  that you are deleting, etc). [#58](https://github.com/rustwasm/walrus/pull/58)
* Added an `id` getter for
  `Import`s. [#59](https://github.com/rustwasm/walrus/pull/59)
* Added a mutable iterator for tables in a
  module. [#59](https://github.com/rustwasm/walrus/pull/59)
* Added a convenience function for getting the main function table for a
  module. [#57](https://github.com/rustwasm/walrus/pull/57)

### Changed

* The `WithSideEffects` expression variant can have arbitrary stack-neutral side
  effects before its value now, in addition to after its
  value. [#55](https://github.com/rustwasm/walrus/pull/55)

--------------------------------------------------------------------------------

## 0.2.1

Released 2019-02-14.

### Added

* Added configuration options for controlling emission of the producers section

--------------------------------------------------------------------------------

## 0.2.0

Released 2019-02-14.

### Added

* Added configuration options for controlling emission of the DWARF and name
  custom sections.

### Changed

* Changed the synthetic naming option from "generate_names" to
  "generate_synthetic_names_for_anonymous_items" to more accurately reflect what
  it does.

--------------------------------------------------------------------------------

## 0.1.0

Released 2019-02-12.

### Added

* Initial release!
