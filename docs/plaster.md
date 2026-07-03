# 🩹 _Plaster_ and semantical patching

_Plaster_ is an experimental tool being introduced in Brave to allow us to apply
changes to upstream sources files, by relying on regex transformations to search
for patterns and apply substitutions.

## Why 🩹 _Plaster_

The two traditional approaches to introduce changes to Chromium have been `git`
patches (i.e. `patches/`), and language overrides (i.e. `chromium_src/`). The
use of patches is in general avoided whenever possible, as they tend to easily
run into conflicts when rebasing Brave onto a newer Chromium version. With
language overrides, although more flexible than patches, they tend to be
invisible in the source code target, hard to interpret, and lead to many cases
of unintended replacements.

Using regexes, _Plaster_ avoids the brittleness of patch files, while providing
matching mechanisms that are more flexible than the methods currently employed
for macro replacement. This also means a language-agnostic way to approach
source changes semantically, and in place. _Plaster_ changes can be both seen in
place, as well as audited as patch files.

## How does it work

_Plaster_ files are placed under `rewrite/`, using a `.yaml` extension, and they
are supposed to match the path for the file being plastered. A deprecated
`.toml` form is also accepted while existing plasters are being migrated — see
[Legacy TOML format (deprecated)](#legacy-toml-format-deprecated) at the end of
this document.

> [!WARNING]
>
> At the moment, only plaster files to sources in Chromium's `src` repo are
> supported.

Each plaster file will be used to apply changes into a given source, and then
generate a patch for the effected changes.

For example, imagine you want to append Brave-specific entries to the upstream
`RequestType` enum in `components/permissions/request_type.h`. You would care
about the following files.

- **Plaster file:** `brave/rewrite/components/permissions/request_type.h.yaml`
- **Source file:** `components/permissions/request_type.h`
- **Patch file:** `brave/patches/components-permissions-request_type.h.patch`

### Creating a plaster file

A plaster file is a YAML file that lists rewrites to be applied to the
corresponding source, based on its path. The following plaster appends
Brave-specific values to the upstream `RequestType` enum.

Creating the source file with `vscode`:

```sh
code rewrite/components/permissions/request_type.h.yaml
```

We can use a regex that anchors on the enum's outer braces and on the existing
`kMaxValue = <something>` line, and then inserts the Brave entries plus a new
`kMaxValue` just before the closing `}`:

```yaml
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

substitutions:
  - description: |
      Append Brave-specific entries to `RequestType`

      This plaster is generic enough to guarantee that our entries are always last,
      and that they also become the new kMaxValue.
    regex:
      re_pattern: '(enum class RequestType \{.+?,)(\s+)kMaxValue = \w+'
      re_flags: [DOTALL]
      replace: |-
        \1
          kWidevine,
          kBraveEthereum,
          kBraveSolana,
          kBraveOpenAIChat,
          kBraveGoogleSignInPermission,
          kBraveCardano,
          kBraveMinValue = kWidevine,
          kBraveMaxValue = kBraveCardano,
          kMaxValue = kBraveCardano
```

_Plaster_ substitutions are listed under `substitutions:`. _Plaster_ loads the
contents of a source from `git`, as the source of truth, not from whatever is on
disk, and then it applies each substitution cumulatively to the contents of the
target source, updating the upstream source at the end.

Each item under `substitutions:` has a type of rewrite (e.g `regex`), and a
`count` field that enforces the number of rewrites a substitution should have.

> [!NOTE]
>
> The default value for `count` is `1` so `count=1` can be omitted, whilst
> `count=0` mean "at least one or more matches".

The default rewrite is a **regex** substitution, whose fields are grouped under
a `regex:` key (as in the example above):

```yaml
substitutions:
  - description: ''
    count: 1 # 1 is the default; 0 means "one or more matches".
    regex:
      # One of either pattern or re_pattern must be specified.
      pattern: '' # non-regex pattern (string will be escaped)
      re_pattern: '' # regex pattern
      replace: ''
      re_flags: [] # traditional Python `re` flag names, e.g. [DOTALL]
```

Use YAML's `|` / `|-` block scalars when you need multi-line `replace` or
`description` values — `|` keeps a trailing newline, `|-` strips it.
Single-quoted YAML strings preserve backslashes literally, which is useful for
regex patterns (`'\s'` stays as the two characters `\s`, not interpreted by
YAML).

### Rewriters

Plaster has the concept of rewriters. Regexes are the most commonly used
rewriter, but there are a few more of them that tend to be more accurate on what
they do (and more should be added going forward).

| Rewrite        | Arguments                   | What it does                                            |
| -------------- | --------------------------- | ------------------------------------------------------- |
| `regex`        | regex fields (above)        | A regex substitution.                                   |
| `make_virtual` | `class_name`, `method_name` | Prepends `virtual ` to a method in a class.             |
| `add_friend`   | `class_name`, `friend_type` | Inserts `friend <friend_type>;` in a private section.   |
| `remove_final` | `class_name`                | Removes the `final` specifier from a class declaration. |

To give an example, the follow substitutions allow a class to be subclassed by
dropping the `final` qualifier from the declaration, and making the destructor
`virtual`.

```yaml
substitutions:
  - description: Drop `final` from DraggingTabsSession to allow subclassing.
    remove_final:
      class_name: DraggingTabsSession

  - description:
      Make the DraggingTabsSession destructor virtual for subclassing.
    make_virtual:
      class_name: DraggingTabsSession
      method_name: '~DraggingTabsSession' # quote: a leading `~` is YAML null
```

### Running plaster

To apply this plaster file, just run `plaster.py`:

```sh
tools/cr/plaster.py apply
```

This will run plaster against the recently changed files. If you want to run for
just one particular file, you can use one of the following formats.

```
tools/cr/plaster.py apply <.yaml file>
tools/cr/plaster.py apply <.patch file>
```

Running `plaster.py` will cause the substitution to be applied in
`components/permissions/request_type.h`, and trigger the creation or update of
the corresponding patch file for this source. For example, it may produce a diff
file like this:

```patch
diff --git a/patches/components-permissions-request_type.h.patch b/patches/components-permissions-request_type.h.patch
new file mode 100644
index 00000000000..bec88027991
--- /dev/null
+++ b/patches/components-permissions-request_type.h.patch
@@ -0,0 +1,20 @@
+diff --git a/components/permissions/request_type.h b/components/permissions/request_type.h
+--- a/components/permissions/request_type.h
++++ b/components/permissions/request_type.h
+@@ -... @@ enum class RequestType {
+   ...,
+   kStorageAccess,
+   kWindowManagement,
+-  kMaxValue = kWindowManagement,
++  kWidevine,
++  kBraveEthereum,
++  kBraveSolana,
++  kBraveOpenAIChat,
++  kBraveGoogleSignInPermission,
++  kBraveCardano,
++  kBraveMinValue = kWidevine,
++  kBraveMaxValue = kBraveCardano,
++  kMaxValue = kBraveCardano,
+ };
```

So at the end, you get a regular patch, and everything works as usual with how
our Brave machinery handles patch files.

### Checking if files are up-to-date

You can verify if all plaster files are applied, with Chromium sources updated
and patch files checked under `path` with the following command:

```sh
tools/cr/plaster.py check
```

This is the equivalent of a dry run of `plaster.py apply`.

### Best practices

See
https://github.com/brave-experiments/brave-core-tools/blob/master/docs/best-practices/plaster.md
