---
name: brave/no-build-gn-beside-sources-gni
description:
  'A directory has either a BUILD.gn (preferred) or a sources.gni, never both.'
rubric: design
severity: should_fix
---

**Rule.** Do not add a `sources.gni` to a directory that has a `BUILD.gn`, or a
`BUILD.gn` to a directory that has a `sources.gni`. Each directory describes its
build in one of the two, preferably `BUILD.gn`.

**Why.** With both files, it is unclear which one is authoritative for the
directory's sources, and dependency tracking becomes harder because some of its
files are built by its own targets and others by upstream targets.

## Examples

Bad:

```text
brave/browser/foo/BUILD.gn     # source_set("foo")
brave/browser/foo/sources.gni  # brave_browser_foo_sources
```

Good:

```text
brave/browser/foo/BUILD.gn     # source_set("foo") with all of foo's files
```
