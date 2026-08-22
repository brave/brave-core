# Build-defaults matrix generator

`bots.py` generates Brave's _generated build defaults_ — the GN values Brave
always overrides in Chromium — as a matrix of per-target `gn-args.json` files,
from a single declarative source, `configs.pyl`. It is a small, standalone
analogue of Chromium's [`//tools/mb/mb.py`](../../../tools/mb/mb.py). Generated
files land under

```
brave/build/defaults/generated/<target>/gn-args.json
```

`configs.pyl` is the source of truth for these defaults: it catalogs both the
`*_defaults.gni` files under `//brave/build/args` and the unconditional,
structural overrides programmed in
[`//brave/build/commands/lib/buildArgs.ts`](../../build/commands/lib/buildArgs.ts).
Secrets, version/channel metadata, signing identity, and CI-only toggles are
deliberately excluded — see the header comment in `configs.pyl` for the full
list and rationale. Each file has the same shape as Chromium's `gn-args.json`,
e.g.:

```json
{
  "gn_args": {
    "is_official_build": true,
    "target_cpu": "x64",
    "target_os": "linux"
  }
}
```

## The model

`configs.pyl` has three sections, with the same shape as upstream
`mb_config.pyl`:

- **`mixins`** — `name -> { 'gn_args': {...}, 'mixins': [<names>] }`. The
  smallest reusable unit. A mixin's own `gn_args` is applied first, then its
  nested `mixins` are applied (so a nested mixin can override the parent's own
  values).
- **`configs`** — `name -> [<mixin names>]`, an ordered list. Mixins are applied
  left to right; on duplicate keys, the later mixin wins.
- **`builder_groups`** — `group -> target -> config name`. The defaults matrix,
  grouped by platform for readability. Each target resolves to exactly one
  config. Target names must be unique across all groups (they become output
  directory names); the group name is not part of the output path.

### Difference from upstream

Upstream expresses `gn_args` as a space-separated string
(`'is_official_build=true target_cpu="x64"'`). Here `gn_args` is a **typed
dict**, so values are real Python literals (`True`/`False`, strings, ints) and
no string parsing is involved.

> [!NOTE] Because values are JSON-serializable literals, raw GN expressions such
> as `foo = bar + baz` cannot be expressed. If a real config needs one, extend
> the tool to support it explicitly rather than smuggling it through a string.

## Adding or changing a target

1. Add/extend `mixins` and `configs` in `configs.pyl` as needed.
2. Map the target to its config under the appropriate `builder_groups` entry.
3. Regenerate and commit the result:

   ```bash
   # from src/brave
   python3 tools/bots/bots.py gen
   ```

## Commands

```bash
# Generate all gn-args.json files.
python3 tools/bots/bots.py gen

# Verify the checked-in files are up to date (exits non-zero if stale).
# Suitable for a presubmit / CI check.
python3 tools/bots/bots.py gen --check

# Parse and flatten every config without writing anything.
python3 tools/bots/bots.py validate

# Run the unit tests.
python3 tools/bots/bots_unittest.py
```

Use `--config <path>` to point at an alternate `configs.pyl`.
