---
name: brave/flag-unretained-only-when-callback-can-outlive-this
description:
  'base::Unretained(this) is flagged only when the callback can outlive this;
  callbacks owned by members destroyed with this (registrars, timers, owned Mojo
  endpoints, subscriptions) are safe.'
rubric: safety
severity: blocking
---

**Rule.** Flag `base::Unretained(this)` only when the callback can outlive
`this`: it is posted to a thread pool or another sequence, posted to a task
runner, stored in a longer-lived object, or handed to an API whose lifetime is
independent of `this`. There, ask for a `WeakPtr` instead. NEVER flag it, and
NEVER request a `WeakPtr` replacement, when the callback is owned by `this` or
by a member destroyed with `this`:

- a `PrefChangeRegistrar` member (including one observing `local_state`);
- a `base::OneShotTimer` / `base::RepeatingTimer` member;
- the disconnect or error handler of a `mojo::Receiver`, `mojo::Remote` or
  `mojo::AssociatedRemote` that `this` owns, unless the binding crosses thread
  boundaries;
- a `base::CallbackListSubscription` stored as a member.

**Why.** A member's destructor unregisters or cancels the callbacks it holds, so
a member-owned callback cannot run after `this` is gone. This is the common,
correct pattern, and flagging every `base::Unretained(this)` produces mostly
false positives. The real use-after-free risk is a callback held by something
whose lifetime is independent of `this`.

## Examples

Safe, do not flag:

```cpp
pref_change_registrar_.Init(prefs_);
pref_change_registrar_.Add(
    kSomePref,
    base::BindRepeating(&MyClass::OnPrefChanged, base::Unretained(this)));

remote_.set_disconnect_handler(
    base::BindOnce(&MyClass::OnDisconnect, base::Unretained(this)));
```

Bad, flag:

```cpp
// Posted to another sequence; `this` may be freed before it runs.
base::ThreadPool::PostTask(
    FROM_HERE, base::BindOnce(&MyClass::DoWork, base::Unretained(this)));
```
