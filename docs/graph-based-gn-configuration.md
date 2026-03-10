# Graph-Based GN Configuration System

## Overview

A declarative, graph-based system for managing GN build configurations.
Developers define named **configs** as TypeScript class methods that compose
into a directed acyclic graph. At build time, a resolver walks the graph
depth-first, merging GN arguments with a simple **"last wins"** rule. The
output is a flat `args.gn` file with direct `key = value` assignments.

Inspired by Chromium's Starlark-based `gn_args.config()` system
(`infra/config/gn_args/gn_args.star`), but implemented in TypeScript to match
the existing Brave build tooling, and designed for both CI builders and local
developer workflows.

## Goals

- **Declarative**: A build configuration is fully described by its config list.
- **Composable**: Configs reference other configs, forming a reusable graph.
  Common patterns are defined once and shared.
- **Type-safe**: Configs are TypeScript class methods. Config references, group
  names, and dynamic args contexts are type-checked, auto-completable, and
  navigable in the IDE. Broken references are caught at compile time.
- **Predictable**: "Last wins" ordering at every level — inside config
  declarations and at the CLI. No implicit priority systems.
- **Reviewable**: Each config is a small, named unit. PRs are easy to review.
- **Vanilla build flow**: After initial setup, `autoninja` works unchanged.
  Re-generation on config file changes is automatic.

## Concepts

### Configs

A config is a named unit of GN arguments, optionally composed from other
configs. Configs are defined as **methods on a `Configs` class** — each method
returns a `ConfigDescriptor<T>`, and the method name is the config name. Config
names are auto-discovered by enumerating the class prototype methods, and
references use `this.methodName` (the resolver identifies configs via each
function's `.name` property).

```ts
type Value = string | number | boolean | { [key: string]: Value }
type Args = Record<string, Value | Value[]>
type ConfigMethod<T> = () => ConfigDescriptor<T>

/** Descriptor returned by each config method. Generic over the class T. */
interface ConfigDescriptor<T> {
  /**
   * Mutual-exclusion group (e.g. 'target_os'). Two configs from the same
   * group in one config list is an error.
   */
  group?: string

  /**
   * When truthy, this config is the default for its group and is automatically
   * applied if no other config from the same group is present. Typically a
   * boolean expression evaluated at load time (e.g. `process.arch === 'arm64'`).
   * Requires `group` to be set.
   */
  isGroupDefault?: boolean

  /**
   * References to other config methods this config depends on.
   * Order matters: later entries override earlier ones for the same GN key.
   * Grouped deps are overridable (skipped when the group is already
   * satisfied); ungrouped deps are always applied.
   * See "Overridable and Hard Dependencies" below.
   */
  configs?: Array<ConfigMethod<T>>

  /**
   * GN argument key-value pairs. Plain object for static args, or a callback
   * `(ctx) => Args` for conditional args. `ctx` is typed as `Partial<T>`
   * — each property is truthy when the corresponding config participates in
   * the build. A config's own args always override args inherited from its
   * `configs` dependencies. See "Dynamic Args" below.
   */
  args?: Args | ((ctx: Partial<T>) => Args)

  /**
   * Marks this config as a checked-in preset. The resolved args are written to
   * `build/args/*.json`, and CI may validate that the selected config still
   * matches the committed file.
   */
  isPreset?: boolean
}
```

Each configs class binds `T` to itself via a type alias. This eliminates
repetition — methods return `Config`, and `ctx` in `args` callbacks is
automatically inferred as `Partial<Configs>`:

```ts
type Config = ConfigDescriptor<Configs>

class Configs {
  linux(): Config {
    return {
      group: 'target_os',
      configs: [this.blink_platform_defaults],
      args: { target_os: 'linux' },
      isGroupDefault: process.platform === 'linux',
    }
  }

  android(): Config {
    return {
      group: 'target_os',
      configs: [this.blink_platform_defaults, this.android_platform_args],
      args: { target_os: 'android' },
    }
  }

  asan(): Config {
    return {
      args: (ctx) => ({
        is_asan: true,
        ...(ctx.android ? { enable_java_asserts: false } : {}),
      }),
    }
  }

  debug(): Config {
    return {
      args: {
        is_debug: true,
      },
    }
  }

  dev_android(): Config {
    return { configs: [this.android, this.debug] }
  }

  ci_android_asan(): Config {
    return {
      configs: [this.android, this.asan],
      isPreset: true,
    }
  }
}

export default new Configs()
```

### Config Composition (The Graph)

Configs form a directed acyclic graph through their `configs` references:

```
ci_android_asan
  ├── android         ← resolved first
  │     (target_os = "android")
  └── asan            ← resolved second, overrides android
        (is_asan = true)
        (enable_java_asserts = false)   ← own args, override everything
```

Resolution is depth-first: for each node, resolve all children (in order), then
apply the node's own args on top. A node referenced by multiple parents is
resolved once and its result is memoized.

### "Last Wins" Rule

The override rule is uniform at every level:

1. **Inside a config declaration**: in `configs: [this.a, this.b, this.c]`, config `c`
   overrides `b`, which overrides `a`. The config's own `args` override all of
   them.
2. **At the CLI**: `--configs=android,debug,asan` means `asan` overrides
   `debug` overrides `android` for any conflicting keys.
3. **User overrides**: args written below the marker in `args.gn` override
   everything.

### Dynamic Args

Most configs use a plain object for `args`. When a config needs different args
depending on what else is in the build, `args` can be a callback receiving a
**context object** (`ctx`) typed as `Partial<Configs>`. Each property is truthy
when the corresponding config participates in the build (directly or as a
transitive dependency), and `undefined` otherwise — so `ctx.android`,
`ctx.release`, `!ctx.debug`, etc. work as natural boolean checks.

See `asan()` and `release()` in the class example above for concrete usage.

To give `ctx` the complete picture, resolution uses **two passes**:

1. **Discovery pass** — walk the full dependency graph from the effective list
   (auto-detected prepends + user configs). Collect all config names that
   participate, including transitive dependencies. Build the `ctx` object from
   this set.
2. **Resolution pass** — DFS with memoization. For each config, evaluate
   `args` (calling it if it's a function, passing `ctx` built in step 1).
   Merge: dependencies → own `args`. "Last wins."

```
Effective list: [component, arm64, android, debug, asan]

Discovery walk collects all participants (including transitive deps of each):
  ctx = {component, arm64, android, debug, asan,
         blink_platform_defaults, android_platform_args, ...}

In asan's args callback:
  ctx.android → truthy  → enable_java_asserts = false
  ctx.win     → undefined (falsy)
```

This works because the graph is static (`configs` dependencies never change) —
the discovery pass finds the same graph regardless of `args` callback results.

Use dynamic args sparingly — most configs are static. Reserve callbacks for
cases where a config genuinely needs different args per platform (e.g.,
sanitizers, toolchain flags).

### Config File Organization

All configs live in a single class in
`brave/build/commands/gn_configs/configs.ts`. This gives one place to see
every config, review changes, and understand ordering. Sections within the
class are separated by comment banners (see example above).

Method declaration order determines group registration order, which in turn
determines auto-detection prepend order (see Groups below).

## Rules

Use a `Rules` class to validate the final combination of resolved args. This is
useful to enforce constraints that depend on the final combination of resolved
args rather than on direct dependency structure:

```ts
class Rules {
  iOSRequiresStaticLinking(ctx: Partial<Configs>, args: Args) {
    if (ctx.ios && args.is_component_build) {
      throw new Error('iOS builds must use static linking')
    }
  }
}
```

## Groups and Auto-Detection

Some configs are mutually exclusive (you can't target both `android` and `mac`)
and have sensible defaults derived from the host environment. This is expressed
directly on the config descriptor returned by each method via two optional
properties:

- **`group`** (string, optional): declares the config as part of an exclusive
  group. Two configs from the same group in the same config list is an error.
- **`isGroupDefault`** (boolean, optional): when truthy, this
  config is automatically applied if no other config from the same group is present.

Group satisfaction checks include transitive dependencies — presets work
correctly: `--configs=ci_pr_android_arm64` (which transitively includes `android`)
satisfies the `target_os` group without needing the user to list it explicitly.

### Default Config Flow

Default configs (those with `isGroupDefault`) are **prepended** to the user's list.
They form the base layer with the lowest priority — any user-provided config
overrides them:

```
User:   --configs=android,debug
Host:   macOS arm64, not CI

Expand user configs (transitive deps included):
  android → deps expanded (blink_platform_defaults, platform args, etc.)
  debug   → no deps
  Expanded set: {android, debug, + android's transitive deps}

Processing defaults in declaration order:

target_os:       "android" in expanded set             → skip (group satisfied)
target_cpu:      none in set, evaluate isGroupDefault:
                 arm64: process.arch === 'arm64' → ✓   → prepended
build_type:      "debug" in expanded set               → skip (group satisfied)
environment:     none in set, evaluate isGroupDefault:
                 ci: isCI → ✗                           → nothing added

Effective: [arm64, android, debug]
            ^^^^^  ^^^^^^^
            group defaults
```

Transitive deps matter — presets satisfy groups through their dependencies:

```
User:   --configs=ci_pr_android_arm64
Host:   macOS arm64

Expand user configs (transitive deps):
  ci_pr_android_arm64 → [android, arm64, static, ci]
  Expanded set: {ci_pr_android_arm64, android, arm64, static, ci}

target_os:       "android" in expanded set             → skip (group satisfied)
target_cpu:      "arm64" in expanded set               → skip (group satisfied)
link_type:       "static" in expanded set              → skip (group satisfied)
build_type:      none in set, no isGroupDefault matches     → nothing added
environment:     "ci" in expanded set, but ci has no group
                 isGroupDefault: isCI                        → skip (already present)

Effective: [ci_pr_android_arm64]
```

Default configs are prepended in method declaration order within the `Configs`
class. Group satisfaction checks include transitive dependencies, so presets
work transparently (their composed configs satisfy groups) and graph
dependencies encode platform constraints (e.g., `ios` → `static`).

## Resolution Algorithm

Given a config list (from CLI or a preset):

1. **Validate names** — check that every config in the list exists in the
   registry. Unknown configs produce a warning locally and an error on CI.
2. **Validate groups** — check that no exclusive group has more than one config
   in the list. Error immediately if violated.
3. **Expand** — walk the user's config list, recursively expanding `configs`
   entries. For each entry, look up the referenced config:
   - **Grouped config**: add only if its group is not yet satisfied in the
     expanded set (by user configs or already-added deps). If satisfied, skip.
   - **Ungrouped config**: add unconditionally.
   - Expand the added config's own `configs` recursively.
   Then validate groups on the expanded set — two configs from the same group
   is an error. Use `gnRule()` to forbid overrides that would otherwise be
   valid (e.g. iOS + component).
4. **Apply global defaults** — process configs with `isGroupDefault` in declaration
   order: for grouped configs, if no group member is in the expanded set,
   evaluate `isGroupDefault` and prepend if truthy (then expand the new default's
   `configs` entries into the set). Same for ungrouped configs. Defaults form
   the lowest-priority base layer.
5. **Discover** — walk the full dependency graph from the effective list
   (steps 1–4). Collect every config name that participates, including
   transitive dependencies. Detect cycles and error if found. This set is
   what `ctx` is built from.
6. **Resolve depth-first with memoization** — traverse the graph again. For
   each node:
   - Resolve all children first (in declaration order).
   - Merge children's resolved args into a dict (later children override
     earlier).
   - Evaluate the node's own `args` — if it's a function, call it with
     `ctx` (built from the full set in step 5); if it's an object, use it
     directly. Merge on top.
   - Memoize the result — if a config is referenced by multiple parents, it's
     resolved once.
7. **Collect the final args** — the virtual root's resolved dict is the
   complete set of GN arguments.
8. **Run post-resolution rules** — execute `gnRule()` validators against the
   final merged args.

## Build Integration

### Initial Setup

The developer runs the wrapper for the first time:

```sh
npm run gn gen -- --configs=android,debug -C out/Debug
```

The wrapper:

1. **Loads `configs.ts`** from `brave/build/commands/gn_configs/`.
2. **Validates groups** and **applies defaults** (prepending `isGroupDefault` matches).
3. **Discovers** the full config set, then **resolves the graph** (evaluating
   `args` callbacks during resolution) — produces a flat dict of GN args.
4. **Writes `args.gn`** in the output directory with flat `key = value`
   assignments and a descriptive header.
5. **Runs real `gn gen`** with appropriate flags.
6. **Patches `build.ninja`** — rewrites the `rule gn` command to point to the
   wrapper script instead of vanilla `gn`.
7. **Patches `build.ninja.d`** — appends all JS files from `require.cache`
   as dependencies (automatically tracks every file used during resolution).

### Generated `args.gn`

```gn
# AUTO-GENERATED by gn config resolver
# @configs: android,debug
#
# Defaults (prepended): component (link_type), arm64 (target_cpu)
# Effective list: [component, arm64, android, debug]

root_extra_deps = ["//brave"]
enable_precompiled_headers = false
is_component_build = true
target_cpu = "arm64"
proprietary_codecs = true
ffmpeg_branding = "Chrome"
target_os = "android"
is_debug = true
symbol_level = 2

# Put your extra args AFTER this line.
```

The `# @configs:` line is a parseable tag — the regen script reads it to know
which configs to resolve. Since it's inside `args.gn`, it survives `gn clean`
(which preserves `args.gn` and its imports).

### `build.ninja` Patching

After `gn gen` produces `build.ninja`, the wrapper prepends the regen script
to the original GN regeneration command, preserving all flags:

Before:

```ninja
rule gn
  command = ../../buildtools/mac/gn --root=../.. -q --regeneration gen .
  description = Regenerating ninja files
```

After:

```ninja
rule gn
  command = node ../../brave/build/commands/scripts/gnRegen.ts ../../buildtools/mac/gn --root=../.. -q --regeneration gen .
  description = Regenerating ninja files
```

The node script receives the entire original GN command as arguments. It:

1. Re-resolves configs and rewrites `args.gn` if needed.
2. Executes the original GN command as a child process if `args.gn` was updated.
3. Re-patches `build.ninja` and `build.ninja.d` (since GN overwrites them).

### `build.ninja.d` Patching

After `gn gen`, the wrapper appends all JS files that were loaded during
config resolution to the deps file. This is done automatically using Node.js's
`require.cache`:

```ts
const deps = Object.keys(require.cache)
  .filter(f => f.startsWith(braveDir))
  .map(f => path.relative(outputDir, f))
```

This captures every file involved in the resolution — `configs.ts`,
`gnConfigRegistry.ts`, `gnConfigResolver.ts`, and any modules they import.
No manual tracking needed; importing a new module anywhere automatically
makes it a regen dependency.

### Regen Entry Point (`gnRegen.ts`)

The regen script, invoked by Ninja with the original GN command as arguments:

```
node gnRegen.ts ../../buildtools/mac/gn --root=../.. -q --regeneration gen .
```

1. Parses the `# @configs:` line from `args.gn` in the output directory.
2. Loads and resolves all configs.
3. Rewrites `args.gn` if the resolved args changed (preserving the
   `# @configs:` line and user overrides below the marker).
4. Executes the original GN command (all arguments after `gnRegen.ts`).
5. Re-patches `build.ninja` (since GN overwrites it).
6. Re-patches `build.ninja.d`.

### User Overrides

Users can add their own overrides below a marker comment in `args.gn`. The
wrapper preserves everything below the marker when regenerating:

```gn
# ... generated args above ...

# Put your extra args AFTER this line.
symbol_level = 0
enable_nacl = false
```

Since user overrides appear after all generated args, they always win (last
wins).

### Automatic Regen via Ninja

Once set up, `autoninja` triggers regen automatically:

| Change | How Ninja detects it | What happens |
|--------|---------------------|--------------|
| Any TS file edited (`configs.ts`, lib files) | File is in `build.ninja.d` (via `require.cache`) | Regen fires → wrapper re-resolves → `args.gn` updated |
| `args.gn` manually edited | Always tracked in `build.ninja.d` | Standard GN regen |

Since `build.ninja.d` is populated from `require.cache`, every file that
participates in resolution is automatically tracked — no manual maintenance.

## Validation

### Mutual Exclusion

The resolver validates group constraints before resolution. If two configs
from the same exclusive group are in the list, the resolver errors with a clear
message:

```
Error: Configs "android" and "mac" are both in exclusive group "target_os".
Only one config per exclusive group is allowed.
```

### Cycle Detection

The graph must be acyclic. If config `a` references `b` which references `a`,
the resolver errors:

```
Error: Cycle detected in config graph: a → b → a
```

### Post-Resolution Validation

After the graph is resolved and the final args dict is produced, custom
validation rules check business invariants. Rules are declared in the same
`configs.ts` file using `gnRule()` (see the "Validation rules" section in the
example above).

Rules run after resolution but before writing `args.gn`. They validate the
**final merged output**, not individual configs. This catches invalid
combinations that individual configs can't prevent.

### Config Name Validation

Config names must be alphanumeric with underscores and hyphens. No spaces, no
special characters. Validated when configs are registered.

## GN Arg Value Formatting

The resolver writes GN args from the resolved TypeScript values. The mapping:

| TS type | GN syntax | Example |
|---------|-----------|---------|
| `string` | `"quoted"` | `target_os = "android"` |
| `number` | bare | `symbol_level = 1` |
| `boolean` (`true`/`false`) | bare | `is_debug = true` |
| `array` | `[...]` | `extra_cflags = ["-g", "-O2"]` |

## File Structure

```
brave/build/commands/
  gn_configs/
    configs.ts                  # Configs class + gnRule() declarations
  scripts/
    gn.ts                       # existing entry point (to be updated)
    gnRegen.ts                  # regen script invoked by Ninja
  lib/
    buildNinjaPatching.ts       # build.ninja and build.ninja.d patching
    gnConfigRegistry.ts         # gnRule() API, class-based config discovery
    gnConfigResolver.ts         # DFS resolution with memoization
    gnArgsWriter.ts             # generates flat args.gn content
```

Everything — configs, groups, auto-detection, args, dependencies, presets, and
validation rules — lives in the `Configs` class (plus `gnRule()` calls) in
`configs.ts`. Method declaration order is the source of truth for group
registration and auto-detection prepend ordering.

## Implementation Plan

1. **Config registry** (`gnConfigRegistry.ts`): `gnRule()` API and class-based
   config discovery. Accepts a `Configs` class instance and enumerates its
   prototype methods to build the config graph. Exports the
   `ConfigDescriptor` and `GnArgs` types. Validates names, detects
   duplicates, discovers groups automatically from config declarations.
2. **Config resolver** (`gnConfigResolver.ts`): Two-pass resolution. Pass 1:
   discovery walk to collect full config set (including transitive deps) and
   detect cycles; builds the `ctx` object. Pass 2: DFS with memoization,
   evaluating `args` callbacks with `ctx` from Pass 1. Group
   mutual-exclusion check. Default prepending (`isGroupDefault`). "Last wins"
   merging. Runs post-resolution validation rules. Pure function, easy to
   unit test.
3. **Args writer** (`gnArgsWriter.ts`): Takes resolved args dict, writes flat
   `args.gn` with header comments showing resolution order. Handles user
   overrides preservation (reads existing `args.gn`, preserves content below
   marker).
4. **Ninja patcher** (`buildNinjaPatching.ts`): Reads `build.ninja`, rewrites
   the `rule gn` command. Reads `build.ninja.d`, appends all files from
   `require.cache` as deps. Writes both back.
5. **Regen entry point** (`gnRegen.ts`): Parses `# @configs:` from
   `args.gn`, loads `configs.ts`, calls resolver, rewrites `args.gn`,
   executes the original GN command (passed as arguments), re-patches
   `build.ninja` and `build.ninja.d`. This is what Ninja invokes.
6. **CLI integration**: Update `gn.ts` to accept `--configs`, orchestrate the
   full initial setup flow.
7. **Seed configs and rules** (`configs.ts`): Populate the `Configs` class
   with initial config methods covering platforms, CPUs, build types,
   sanitizers, and CI presets, with appropriate `group`, `isGroupDefault`, and
   dynamic `args` callbacks. Add `gnRule()` validators for known invariants.
