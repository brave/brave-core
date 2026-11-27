# Test isolates

A test isolate is a self-contained archive with gtest-based test binaries, their
runtime dependencies and a runner, to run tests on macOS and Windows hosts
without a checkout or a build.

- [`generator/`](generator) creates isolates (on Linux only).
- [`runner/`](runner) is copied into isolates and runs their tests.

## Creating an isolate

```sh
pnpm test <test>[,<test>...] [build_config] --isolate=<file>.tar.xz [test args...]
```

`<test>` is a test target or a test group, by name or gn label. Groups that
write `<out_dir>/<group>.json`, such as `brave_all_tests` and
`chromium_all_tests` (see [`brave/BUILD.gn`](../../BUILD.gn)), are expanded into
their tests. Test args are stored in the isolate. `--isolate_data_dir=<name>`
sets the name of the data directory (`data` by default).

## Archive layout

```
run.sh or run.bat         Entrypoint: run.bat for Windows targets, run.sh
                          otherwise.
manifest.json             Manifest of the suites to run.
runner/...                The runner.
data/src/...              Test binaries and their runtime dependencies, at
                          their paths in the checkout. The directory name is
                          set by --isolate_data_dir.
```

## Running an isolate

```sh
tar -xJf tests.tar.xz
./run.sh [test args...]      # macOS, Linux
run.bat [test args...]       # Windows
```

The entrypoint checks that Node.js 26 or newer is in `PATH`, then runs the
suites one by one, with the stored args followed by the given ones. It stops at
the first failing suite and exits with its exit code.

## Runner constraints

The runner runs without `node_modules`, so it may only use Node.js built-ins
(enforced by `runner/tsconfig.json` and ESLint) and erasable TypeScript syntax.
Its tests use the Node.js test runner, as Jest can't load modules that use
`import.meta`:

```sh
node --test build/isolate/runner/*.test.ts
```
