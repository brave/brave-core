---
name: fix-bp-docs
description:
  'Audit and fix best practices docs for stale references, duplicates, obsolete
  content, and formatting issues. Triggers on: fix bp docs, fix best practice
  docs, audit bp docs.'
allowed-tools:
  Bash(find:*), Bash(ls:*), Bash(head:*), Bash(git:*), Bash(python3:*), Read,
  Grep, Glob, Edit
---

# Fix Best Practices Docs

Audit all best practices documentation files for issues and fix them. This skill
must be run from the `brave-core` directory.

---

## Step 1: Paths

This skill runs from the brave-core (`src/brave`) directory. Best practices docs
are at `./docs/best-practices/` and the index is at `./docs/BEST-PRACTICES.md`.

Set `DOCS_DIR="./docs/best-practices"`.

---

## Step 2: Read All Best Practice Files

Read `docs/BEST-PRACTICES.md` and every file in `$DOCS_DIR/*.md` to understand
the full set of docs.

---

## Step 3: Check for Duplicates

Scan across all docs for rules that cover the same topic. Look for:

- Rules with identical or near-identical bold principle statements
- Rules with overlapping code examples covering the same pattern
- Rules that were accidentally added to multiple category files

When a duplicate is found, keep the version in the more appropriate category
file and remove the other. Preserve all anchor IDs on the kept version so
existing links don't break.

Do NOT create reference-only stubs that just point to another rule. Either keep
the full rule or remove it entirely.

---

## Step 4: Check for Stale Brave File References

Extract all `brave/...` file paths from code examples and verify they exist in
`src/brave/`. For each missing file:

1. Search for the renamed/moved file using `find` or `Glob`
2. Update the reference to the correct current path
3. If the file was deleted entirely and no replacement exists, update the
   example to use a file that does exist

Skip paths that are obviously placeholder/illustrative (e.g.,
`brave/components/my_feature/my_header.h`,
`brave/chromium_src/path/to/override.h`).

---

## Step 5: Check for Stale Chromium File References

Extract Chromium file paths (e.g., `components/omnibox/browser/...`,
`chrome/browser/ui/...`) from code examples and verify they exist in `src/`. For
each missing file:

1. Search for the renamed/moved file
2. Update the reference

Skip paths that are obviously placeholder/illustrative.

---

## Step 6: Check for Stale Chromium Symbols

Look for references to specific Chromium class names, method names, and other
symbols in code examples. Verify they still exist in the Chromium source
(`src/`). Update any that were renamed.

---

## Step 7: Check for Stale Brave Symbols

Look for references to specific Brave class names, method names, and other
symbols in code examples. Verify they still exist in `src/brave/`. Update any
that were renamed or removed.

---

## Step 8: Check for Broken Internal References

Look for:

- Links to other docs or files that don't exist (e.g.,
  `../testing-requirements.md`)
- References to anchor IDs that don't exist in the target file
- Links to external URLs that are clearly wrong (but don't fetch them)

---

## Step 9: Check for Formatting Issues

Look for:

- Missing `---` separators between rules
- Missing anchor IDs (`<a id="...">`)
- Duplicate `---` separators
- Inconsistent heading levels

---

## Step 10: Check the Index

Verify `docs/BEST-PRACTICES.md`:

- All listed docs exist
- No doc is listed twice
- All docs in `$DOCS_DIR/` are represented in the index

---

## Step 11: Report and Fix

For each issue found:

1. Describe the issue clearly
2. Fix it directly using Edit
3. Move on to the next issue

After all fixes, run ID validation:

```bash
python3 ./script/manage-bp-ids.py --validate
```

---

## Important Guidelines

- **No Claude Code attribution** in commits or PRs
- **Never remove anchor IDs** even when removing duplicate rules — move them to
  the kept version
- **Never create reference-only stubs** that just point to another rule
- **Preserve the style** of each document when making edits
- **Keep examples realistic** — when updating a stale file path, pick a real
  file that makes sense in context
- When a code example uses a placeholder path (like `path/to/foo.h`), leave it
  alone
