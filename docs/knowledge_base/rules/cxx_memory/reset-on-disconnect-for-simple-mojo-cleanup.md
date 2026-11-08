---
name: brave/reset-on-disconnect-for-simple-mojo-cleanup
description:
  'A Mojo remote whose disconnect handler would only reset it uses
  remote.reset_on_disconnect() instead.'
rubric: readability
severity: nit
---

**Rule.** When the only thing a `mojo::Remote`'s disconnect handler does is
reset that remote, call `remote_.reset_on_disconnect()` instead of setting a
disconnect handler.

**Why.** The built-in call does the same thing without a handler method and a
bound callback to maintain.

## Examples

Bad:

```cpp
remote_.set_disconnect_handler(
    base::BindOnce(&MyClass::OnDisconnect, base::Unretained(this)));
void OnDisconnect() { remote_.reset(); }
```

Good:

```cpp
remote_.reset_on_disconnect();
```
