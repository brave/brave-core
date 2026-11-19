---
name: brave/python-file-writes-use-lf-newline
description:
  'A Python script opening a file for writing passes newline=''\n'', so its
  output has LF line endings on every platform.'
rubric: correctness
severity: should_fix
---

**Rule.** Every `open()` of a file for writing (mode `'w'`, `'a'`, `'x'` or
`'+'`, in text mode) in a Python script passes `newline='\n'`.

**Why.** Without it, Python writes `\r\n` on Windows, so generated files differ
between platforms: they defeat build caching, fail byte-for-byte comparisons and
show spurious diffs.

## Examples

Bad:

```python
with open(output_path, 'w') as f:
    f.write(content)
```

Good:

```python
with open(output_path, 'w', newline='\n') as f:
    f.write(content)
```
