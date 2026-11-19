---
name: brave/python-build-scripts-exit-with-main
description:
  'Python scripts used in the build define main() and run it as sys.exit(main())
  under the __main__ check.'
rubric: readability
severity: should_fix
---

**Rule.** A Python script used in the build system puts its work in a `main()`
function and runs it with `sys.exit(main())` inside
`if __name__ == '__main__':`. Do not run work at module level, and do not call a
bare `main()` whose return value is dropped.

**Why.** `sys.exit(main())` turns the value `main()` returns into the process
exit code, and an uncaught exception still exits non-zero, so the build sees
every failure. A bare `main()` exits 0 whatever it returns, which hides failures
from GN actions and hooks; module-level work also runs when the file is merely
imported.

## Examples

Bad:

```python
if __name__ == '__main__':
    main()
```

Good:

```python
import sys

def main():
    ...

if __name__ == '__main__':
    sys.exit(main())
```
