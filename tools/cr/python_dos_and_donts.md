# Python dos and donts

A few module-wide conventions worth following when adding code under `tools/cr`.

## Type annotations

Always use type annotations whenever possible. Prefer the PEP 604 / PEP 585
modern forms over the legacy `typing` imports.

Don't:

```python
from typing import Dict, List, Optional, Tuple, Union

def f(x: Optional[str]) -> Tuple[int, int]: ...
def g(items: List[Path], lookup: Dict[str, Commit]) -> None: ...
def h(value: Union[int, str]) -> None: ...
```

Do:

```python
def f(x: str | None) -> tuple[int, int]: ...
def g(items: list[Path], lookup: dict[str, Commit]) -> None: ...
def h(value: int | str) -> None: ...
```

For modules that need to reference a class that's declared later in the same
file (forward references in annotations), add
`from __future__ import annotations` at the top of the file rather than quoting
individual annotations as strings.

## Paths

Use `pathlib.Path` for any path work, and annotate parameters that take paths as
`Path` rather than `str`.

Avoid `os` / `os.path` when there's a `Path` equivalent that is as easy to
understand as the `os.path` equivalent. However there's no need to be
overzealous about avoiding `os.path.join`. For example `os.path.relpath` can be
a better fit than `Path` for certain things. `os.path.join` may be appropriate
when dealing with pure strings.

Don't:

```python
src = os.path.join(root, 'src', 'brave')
if os.path.isdir(src):
    parent = os.path.dirname(src)
```

Do:

```python
src = root / 'src' / 'brave'
if src.is_dir():
    parent = src.parent
```

## Reading and writing files

The tooling here is cross-platform, and the files it touches must be
byte-identical on each platform.`Path.read_text()` / `write_text()` defaults
break that contract on Windows because:

- Encoding defaults to the platform locale (`cp1252` on Windows, `utf-8`
  elsewhere), Windows and utf-8 on Linux, which can lead to Windows-only test
  failures.
- Universal-newline mode translates `\n` ↔ `\r\n` and rewrites the bytes on
  disk.

Always pass `encoding='utf-8'`, and disable newline translation: use
`read_bytes().decode('utf-8')` for reads and pass `newline=''` to `write_text`
for writes.

Don't:

```python
text = todo_file.read_text()
todo_file.write_text(content)
```

Do:

```python
text = todo_file.read_bytes().decode('utf-8')
todo_file.write_text(content, encoding='utf-8', newline='')
```

Skip the newline workaround only when you actually want platform-native line
endings in the output.

Furthermore, always prefer `pathlib.Path` methods over the older
`open(path) as f: f.read()` / `f.write(...)` idiom for whole-file reads and
writes -- they're one-liners, don't need a `with` block, and integrate with the
rest of the path manipulation we already do with `Path` objects.

Don't:

```python
with open(path, 'r', encoding='utf-8') as f:
    text = f.read()

with open(path, 'w', encoding='utf-8') as f:
    f.write(text)
```

Use the `Path` pattern shown above instead.

When streaming is required (e.g. processing a large file in chunks), use
`Path.open()` with a `with` block:

```python
with path.open('r', encoding='utf-8') as file:
    for chunk in iter(lambda: file.read(4096), ''):
        checksum.update(chunk.encode())
```

## Classes with data

Prefer `@dataclass` over hand-rolled `__init__` / `__repr__` / `__eq__`
boilerplate. Pick the variant that matches how the data is meant to flow:

- **`@dataclass(frozen=True)`** when instances are read-only after construction.
  Fields are public, but the `frozen=True` setting blocks mutation -- callers
  can read `obj.field` directly without an accessor.
- **Plain class or `@dataclass` with private members and `@property` accessors**
  when the object has internal invariants to keep in sync, or only a subset of
  fields should be mutable from outside. Store state as `_field`, expose reads
  via `@property`, and add `@field.setter` only for the fields that genuinely
  need outside writes. A setter is also the natural place to refresh derived
  state.
- **`@dataclass` (non-frozen)** is fine when the object is a plain mutable bag
  with no invariants -- e.g. a parsed-then-rewritten container whose fields are
  independent. If you find yourself wanting "this field is read-only but that
  one isn't", switch to the private-members pattern.

When deciding whether to use a `@dataclass` or a _plain class_, consider the
following:

- (_Preferable when possible_) A **`@dataclass`** declares fields in the class
  body, which makes it easy to see and document field declarations. It also
  generates `__init__` / `__repr__` / `__eq__` on its own. In our code,
  `@dataclasses` should always be used with composition, and if inheritance is
  needed, then _plain classes_ should be used instead.
- A **plain class** declares its fields inside the constructor body as
  `self.field = ...` assignments in `__init__`, rather than at the class body.
  That gives you full control over the constructor signature (positional args,
  keyword-only args, validation, computed defaults) and supports field
  inheritance from a base class.

Types annotated with `@dataclass` (and `@dataclass(frozen=True)`) can make use
of factory methods (`@staticmethod`, `@classmethod`, etc) as an alternative to
the absence of a constructor.

Don't (public attributes on a hand-rolled class, no protection against external
mutation):

```python
class Config:
    def __init__(self, name: str, retries: int):
        self.name = name
        self.retries = retries
```

Do (frozen dataclass -- public reads, no mutation):

```python
@dataclass(frozen=True)
class Config:
    name: str
    retries: int
```

Do (private members + accessors when some fields need controlled writes):

```python
class EntryLine:
    def __init__(self, *, command: str, hash: str) -> None:
        self._command = command
        self._hash = hash
        self._out = f'{command} {hash}'

    @property
    def hash(self) -> str:
        return self._hash

    @property
    def command(self) -> str:
        return self._command

    @command.setter
    def command(self, new_command: str) -> None:
        self._command = new_command
        self._refresh_out()
```
