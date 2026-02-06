# Test Tools

## check_chromium_test_filters.py

Validates Chromium test filter files in `test/filters/` against the source
tree. Identifies renamed tests and stale entries that no longer correspond to
any test in `src/`.

### How it works

1. Builds an index of all test definitions by scanning `.cc`/`.mm`/`.cpp`
   files for standard gtest macros (`TEST`, `TEST_F`, `TEST_P`,
   `IN_PROC_BROWSER_TEST_F`, `IN_PROC_BROWSER_TEST_P`, etc.)
2. Handles `MAYBE_`/`DISABLED_`/`MANUAL_` prefixed names by stripping the
   prefix (these are compile-time macros)
3. Checks each filter entry against the index
4. For missing entries where the class no longer exists, searches for a
   similarly-named class that has the same method (detects upstream class
   renames)
5. Removes stale entries and cleans up orphaned comments, duplicates, and
   entries already covered by wildcards

### Usage

Dry run (report only):
```
python3 tools/test/check_chromium_test_filters.py
```

Apply fixes:
```
python3 tools/test/check_chromium_test_filters.py --fix
```

### When to run

Run this after a Chromium upgrade to clean up filter files. The script should
be run from the `brave/` directory (or anywhere, since it resolves paths
relative to its own location).
