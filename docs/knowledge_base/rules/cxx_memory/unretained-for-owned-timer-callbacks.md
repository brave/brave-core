---
name: brave/unretained-for-owned-timer-callbacks
description:
  'A callback for a timer the class owns may bind base::Unretained(this); a
  WeakPtr there is a style choice and is NEVER reported as a bug.'
rubric: readability
severity: optional
---

**Rule.** When the class owns the `base::OneShotTimer` or `base::RepeatingTimer`
it starts, `base::Unretained(this)` is the preferred receiver for the timer's
callback. A `WeakPtr` receiver there only adds overhead: you may mention it as a
style preference, but NEVER report it as a correctness bug and NEVER insist on
changing it.

**Why.** The timer is destroyed with the class, so it can only fire while `this`
is valid. This is the opposite of the thread-pool case because of ownership: an
owned timer cannot outlive its owner, whereas a posted task can.

## Examples

Acceptable, but not preferred (not a bug):

```cpp
timer_.Start(FROM_HERE, delay,
    base::BindRepeating(&MyClass::OnTimer, weak_factory_.GetWeakPtr()));
```

Preferred:

```cpp
timer_.Start(FROM_HERE, delay,
    base::BindRepeating(&MyClass::OnTimer, base::Unretained(this)));
```
