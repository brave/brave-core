# Code review knowledge base

This directory holds Brave's knowledge base: the code review rules that an
automated reviewer checks a change against. Each rule is a short Markdown file
stating one convention in a form that can be matched to the lines a change adds,
and each directory's `COMMON_METADATA` says which changes need which rules. A
review therefore loads only the rules a change can break, which costs fewer
tokens and gives the reviewer less to misapply.

The format is that of Chromium's knowledge bases (`agents/knowledge_base/` and
the per-directory ones), so the same tooling reads Brave's rules and Chromium's,
and a review of a Brave change applies both.

The rules hold every convention of [`docs/best-practices/`](../best-practices/),
which the knowledge base is to replace. Until it does, a convention added to
those documents is added here as well.

## Layout

```
docs/knowledge_base/
  README.md
  rules/
    <topic>/
      COMMON_METADATA
      <slug>.md
```

| Directory                    | Conventions for                                              |
| ---------------------------- | ------------------------------------------------------------ |
| `android/`                   | Android Java and Kotlin, resources, bytecode adapters        |
| `architecture/`              | Layering, dependency injection, factories, keyed services    |
| `build_system/`              | GN, DEPS, GRD resources, build scripts                       |
| `buildflags/`                | `#if BUILDFLAG(...)` guards in C++                           |
| `chromium_src/`              | `chromium_src/` overrides                                    |
| `cxx/`                       | C++ naming, includes, `CHECK`s, comments, logging            |
| `cxx_apis/`                  | `//base` utilities, containers, types, callbacks, Rust FFI   |
| `cxx_memory/`                | Ownership, `WeakPtr`, `raw_ptr`, threading, service shutdown |
| `documentation/`             | Comments, READMEs and Markdown documents                     |
| `frontend/`                  | WebUI in TypeScript and React                                |
| `ios/`                       | Swift, SwiftUI, UIKit and Objective-C bridges                |
| `localization/`              | GRD strings, placeholders, translations                      |
| `nala/`                      | Leo and Nala icons, colours and components                   |
| `patches/`                   | `patches/` files                                             |
| `plaster/`                   | `rewrite/` plasters                                          |
| `style_guide/`               | Voice, capitalisation, punctuation, product names in UI text |
| `testing_async/`             | Waiting for asynchronous work in tests                       |
| `testing_isolation/`         | Fakes, test structure and coverage                           |
| `testing_javascript/`        | JavaScript in browser tests                                  |
| `testing_navigation/`        | Navigation and timing in tests                               |
| `testing_upstream_failures/` | Upstream test failures and filter files                      |
| `ui_views/`                  | Desktop views                                                |

## Rule format

````markdown
---
name: brave/<slug>
description: <one sentence: when the rule applies, and what it asks>
rubric: testing
severity: blocking
---

**Rule.** What to do or not do, precisely enough to be checked against the lines
a change adds.

**Why.** The rationale: what goes wrong otherwise.

## Examples

Bad:

```cpp
// A minimal snippet showing the anti-pattern.
```

Good:

```cpp
// The same snippet, fixed.
```
````

- **`name`** is `brave/` followed by the file's stem. A reviewer cites the rule
  by it (`[brave/no-run-until-idle]`), so it is unique across the knowledge
  base, and it never clashes with Chromium's names, which start with their
  directory (`base/`, `general/`, `ios/`).
- **`description`** is the sentence a reviewer reads before deciding whether to
  read the rest.
- **`rubric`** is one of `correctness`, `safety`, `security`, `design`,
  `readability`, `testing`, `perf` or `change_scope`.
- **`severity`** is one of `blocking`, `should_fix`, `nit` or `optional`.
  `blocking` is kept for what a rule states as must or never, and for what
  prevents crashes, use-after-free, security and privacy problems, and flaky
  tests. Style conventions are `should_fix` or `nit`.

The body has the **Rule.** and **Why.** paragraphs, then `## Examples`, as
Chromium's rules do. Further conventions:

- One convention per rule, stated once: a convention two rules would share
  belongs in one of them.
- Prose wraps at 80 columns, with symbols in backticks.
- Examples use real Brave or Chromium APIs, or obvious placeholders
  (`FooService`, `kFooFeature`), and follow the language's style guide.
- State negative boundaries emphatically (`NEVER`, `MUST NOT`): a reviewer reads
  mild phrasing as optional.

## Metadata format

Each directory's `COMMON_METADATA` is a text-format protobuf with one
`agentic_review` block, whose `capability_groups` name the rules they apply:

```
agentic_review {
  # Waits on idle task queues, in C++ tests.
  capability_groups {
    path_expression: "(?:.*/)?[^/]*(?:_unittest|_browsertest|_test)\\.(?:cc|mm)"
    content_patterns: "\\bRunUntilIdle\\s*\\("
    rules: "docs/knowledge_base/rules/testing_async/no-run-until-idle.md"
  }
}
```

A group applies to a change when one file the change touches meets all of its
file fields. Paths are relative to the root of brave-core, and patterns are
Python regular expressions in the RE2 subset Chromium writes; path expressions
must match the whole path.

| Field                       | The group applies when the file...                                                                                                                             |
| --------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `path_expression`           | has a path one of these fully matches                                                                                                                          |
| `path_expression_exclusion` | has a path none of these fully matches                                                                                                                         |
| `content_patterns`          | has an added line in which one of these is found                                                                                                               |
| `diff_patterns`             | has a changed line, written with its `+` or `-` marker, in which one of these is found (`^-` finds a removal)                                                  |
| `required_diff_patterns`    | has, for each of these, a changed line in which it is found                                                                                                    |
| `ast_patterns`              | holds code matching one of these [ast-grep] patterns spanning an added line, as the change leaves the file (`ast_language` names a language the path does not) |

`content_patterns`, `diff_patterns` and `ast_patterns` together select the lines
a match reports; a file in which they select none does not count. A group with
no file field applies to every change, and `rules` and
`review_capability_skills` name what it applies. The remaining fields
(`overridden_by_brave`, `bytecode_adapted`, `brave_mentions`,
`subject_patterns`, `body_patterns`, `subject_pattern_exclusion` and `tool`)
serve the upstream sweeps of `tools/cr/codehealth/`, and are described in
[`tools/cr/knowledge_base.py`](../../tools/cr/knowledge_base.py).

### Scoping a rule

A group's scope decides what a review costs, and how often its rules are read
against code they say nothing about.

- Give every group a `path_expression` for the files its rules can apply to:
  `(?:.*/)?[^/]*\\.(?:cc|h|mm)` for C++, `(?:.*/)?BUILD\\.gn|.*\\.gni` for GN,
  `(?:.*/)?[^/]*_(?:unit|browser)test\\.cc` for C++ tests.
- When a rule concerns particular APIs, macros or idioms, add the
  `content_patterns` that find them in added lines, so that the rule is loaded
  only for a change using them. A pattern only has to find the lines a violation
  could be on: the reviewer decides whether it is one.
- Prefer an `ast_patterns` entry when a regular expression cannot tell the
  construct apart, such as a call with particular arguments.
- Leave out content patterns only for rules about the shape of any code in the
  files they scope (naming, comments, structure). Such rules are read for every
  change to those files, so keep them few.
- Rules with the same scope share one group.

## How a review uses it

`tools/cr/knowledge_base.py` decides which rules a change needs:

```sh
# The rules the current branch needs, against its merge base with origin/master.
vpython3 tools/cr/knowledge_base.py match
# The rules a pull request's diff needs, reading its files at its head commit.
vpython3 tools/cr/knowledge_base.py match pr.diff --head <sha>
```

It prints, as JSON, each rule a change needs, most severe first, with the file
holding it, its URL, a one-sentence `summary`, and the changed lines of each
file that matched. A match is `targeted` when a group's content, diff or AST
patterns found the lines the rule concerns, and broad when only the paths
selected it.

The `review-prs` skill, with its `kb` argument, builds the reviewer's prompts
from it. Targeted rules, and all of Brave's, go in full into prompts that hold a
share of the rules each, with the changed lines that made each one apply and the
diff of only the files they matched. Chromium's broad rules, of which a C++
change selects several hundred, are listed in one further prompt by their
summary, for the reviewer to open the few the diff may break. This is why a
Brave rule's scope matters: a rule without content patterns is read in full for
every change to its files.

Each file of a Brave change is matched against two knowledge bases:

- **Brave's**, this directory, under its path in brave-core.
- **Chromium's**, `agents/knowledge_base/rules/` in `src/`, under its path in
  Chromium's tree, `brave/<path>`. Chromium's general rules (C++, Java, Rust,
  documentation) apply to every Brave file in their language. A file that
  overrides an upstream file is also matched as that file, under its upstream
  path and the DIR_METADATA of its directory: a `chromium_src/` override with
  its own lines, a `rewrite/` plaster with its YAML, and a `patches/` patch with
  the lines it adds to the file it patches. Chromium's rules for, say,
  `components/sync/` therefore review Brave's overrides of it.

DIR_METADATA files scope `agentic_review` to their directory as `dirmd` computes
it: a directory inherits its ancestors' groups, imports its `mixins`, and drops
the rules its `exclude_rules` names. Each repository resolves its own paths, so
a nested repository's metadata (such as `third_party/devtools-frontend/src/`)
matches paths relative to its root.

Brave's rules take precedence over Chromium's: where following a Chromium rule
would break a Brave one, the reviewer does not report the Chromium rule. This
settles the conventions Brave's override mechanisms impose on code that
Chromium's rules also see under its upstream path, such as a `chromium_src` hook
that only forwards to Brave code, or a patch that comments out what it would
otherwise delete.

Where Chromium's metadata does not suit Brave, it is changed by a plaster under
`rewrite/agents/knowledge_base/`, like any other upstream file: a group whose
rules Brave's equivalent of an upstream directory should also have is widened,
and a rule that is wrong for any Brave code is dropped. Chromium's rules are
dropped when they concern infrastructure only Google's Chrome has (its updater,
field trial configurations, Gerrit and its trybots, Chrome branding), when only
a commit message could break them (reviews read diffs), and when they contradict
a Brave convention everywhere they apply.

## Checking the knowledge base

```sh
vpython3 tools/cr/knowledge_base.py lint
```

`lint` checks every knowledge base in brave-core (this one and
`tools/cr/codehealth/rules/`), and the Chromium metadata reviews load. It
reports metadata that does not parse, unknown fields, patterns that do not
compile, groups naming missing files, rules no group names, duplicate names, and
rules without `name`, `rubric` or `severity` or without their **Rule.** and
**Why.** paragraphs. The presubmit runs it for changes to this directory.

## Adding or changing a rule

1. Look for a rule on the topic first (`grep -r` over `rules/`), and sharpen it
   rather than add a second rule that fires on the same lines.
2. Write the rule in the directory of its topic, in the format above.
3. Name it from a capability group of that directory's `COMMON_METADATA`, scoped
   as tightly as its convention allows. Check the scope with `match` on a diff
   that breaks the rule and on one that does not touch what it concerns.
4. Run `lint`.

## Chromium's knowledge bases

For reference, the parts of Chromium's system this one follows:

- The schema is the `AgenticReview` message of `dir_metadata.proto`, in
  `infra/go/src/infra/tools/dirmd/proto/`. `dirmd validate-agentic-review`
  checks that referenced files are tracked, that subagent manifests parse, and
  that patterns compile as RE2.
- `agents/knowledge_base/rules/` is import-only: a directory opts in by listing
  a rules directory's `COMMON_METADATA` in its DIR_METADATA `mixins`, next to an
  `agentic_review {}` block. Several components keep their own knowledge base
  next to their code (for example `net/device_bound_sessions/knowledge_base/`)
  and wire it from their own DIR_METADATA.
- Rules carry `name`, `rubric` and `severity`, and in newer knowledge bases a
  `description` and `citations` (`path`, `heading`, `quote`) of the
  documentation a rule restates. Brave's rules are that documentation
  themselves, and cite none.
- The reviewer that reads them is Google's, run on Gerrit changes. Brave's
  tooling reproduces the matching it implies, with these readings of the schema:
  a group applies when a single file meets its path and content fields, and
  `content_patterns` are matched against an added line without its `+` marker.

[ast-grep]: https://ast-grep.github.io/guide/pattern-syntax.html
