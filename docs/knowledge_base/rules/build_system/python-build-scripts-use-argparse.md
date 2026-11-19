---
name: brave/python-build-scripts-use-argparse
description:
  'Python scripts used in the build parse their command line with argparse,
  never by reading sys.argv directly.'
rubric: readability
severity: should_fix
---

**Rule.** A Python script used in the build system (a GN `action()` script, a
`DEPS` hook, a build or utility script) declares its arguments with
`argparse.ArgumentParser` and reads them from `parse_args()`. It MUST NOT index,
slice or scan `sys.argv` itself.

**Why.** Hand-parsed `sys.argv` breaks on ordinary command-line forms
(`--flag=value` and `--flag value`, optional arguments, reordering), validates
nothing and offers no `--help`, so a GN action that passes its arguments
slightly differently fails in confusing ways.

## Examples

Bad:

```python
def main():
    output = sys.argv[1]
    inputs = sys.argv[2:]
```

Good:

```python
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', required=True)
    parser.add_argument('inputs', nargs='+')
    args = parser.parse_args()
```
