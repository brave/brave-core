# Brave IWYU tooling

Two scripts that drive [include-what-you-use](https://include-what-you-use.org/)
over Brave sources:

* `build_iwyu.py` — builds the IWYU binary against Chromium's pinned Clang.
* `run_iwyu.py` — runs IWYU against an existing Brave build dir and applies
  the suggested include fixes in place.

IWYU is **not** part of the normal Brave build; run these scripts on demand.

## One-time setup: build IWYU

```sh
vpython3 tools/cr/iwyu/build_iwyu.py
```

Clones LLVM at Chromium's pinned `CLANG_REVISION` and IWYU at the pinned
`IWYU_REVISION` into `out/iwyu/`, then builds:

```
out/iwyu/tools/clang/third_party/llvm/build/bin/include-what-you-use
```

Re-run after Chromium's Clang pin moves, or bump `IWYU_REVISION` in
`build_iwyu.py` to pick up an upstream IWYU fix.

## Enable the paths you want IWYU to scan

Edit `brave/build/include_what_you_use_paths.cfg`. Each non-comment line is:

```
+path/to/enable
-path/to/disable
```

Paths are relative to `src/`. Longest-matching prefix wins, so you can
enable a directory and carve exceptions out of it:

```
+brave/components/brave_ads/browser
-brave/components/brave_ads/browser/third_party
```

## Run IWYU

IWYU needs an existing Brave build in some `out/<config>` to pick up its
compile flags and generated headers, **and** several GN args have to be
flipped — without them IWYU crashes during analysis:

```sh
npm run build -- Static \
  --target_os=android \
  --ignore_compile_failure \
  --target=brave:all \
  --gn=clang_use_chrome_plugins:false \
  --gn=force_enable_raw_ptr_exclusion:true \
  --gn=enable_precompiled_headers:false \
  --gn=treat_warnings_as_errors:false \
  --gn=clang_warning_suppression_file:"" \
  --gn=symbol_level:0
```

Required (IWYU crashes without these):

* `clang_use_chrome_plugins:false`.
* `force_enable_raw_ptr_exclusion:true`
* `enable_precompiled_headers:false`
* `treat_warnings_as_errors:false`

Then finally run `include-what-you-use` with `run_iwyu.py`, bearing in mind that `--out` is relative to `src/`, rather than the current directory.

```sh
vpython3 tools/cr/iwyu/run_iwyu.py --out out/Static --verbose
```

This:

1. Generates a compile database from `out/<config>` and filters it to
   sources enabled by `include_what_you_use_paths.cfg`.
2. Runs `iwyu_tool.py` with our libc++ mapping file
   (`brave/build/include_what_you_use_mappings.json5`).
3. Applies suggestions in place via `fix_includes.py`.
4. Strips blackholed includes (e.g. `<new>`) from modified files.
5. Runs `npm run format` so the resulting diff matches Brave's style.

Artifacts written next to the build:

* `out/<config>/iwyu_compile_commands.json` — filtered compile DB.
* `out/<config>/iwyu_suggestions.txt` — raw IWYU output, handy when a
  rewrite looks wrong.

## Defending an include from IWYU

If IWYU keeps removing an include you need, mark it:

```c++
#include "foo/bar.h"  // IWYU pragma: keep
```

`run_iwyu.py` leaves any line containing `IWYU pragma:` untouched, including
during the blackhole-includes pass.
