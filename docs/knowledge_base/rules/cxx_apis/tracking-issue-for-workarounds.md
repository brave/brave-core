---
name: brave/tracking-issue-for-workarounds
description:
  'Temporary workaround or hack code must carry a
  TODO(https://github.com/brave/brave-browser/issues/<id>) comment saying when
  and why it can be removed; permanent design decisions need no tracking issue.'
rubric: readability
severity: blocking
---

**Rule.** Code added as a temporary workaround or hack (to sidestep an upstream
bug, a race, a missing API, a not-yet-shipped dependency) MUST be preceded by a
`TODO(https://github.com/brave/brave-browser/issues/<id>)` comment that says
what is being worked around and when the code can be removed. A `HACK`, `FIXME`
or "temporary" comment without such an issue does not satisfy the rule.

NEVER require a tracking issue for a permanent design decision: a comment that
explains why another API or approach was not used, where the current code is the
intended long-term solution, is a design rationale, not a workaround.

**Why.** Workarounds without a tracking issue are never revisited and become
permanent technical debt; the issue records the condition for removing them and
lets someone find them when it is met.

## Examples

Bad:

```cpp
// HACK: skip validation for now
if (ShouldSkipValidation()) return;
```

Good:

```cpp
// TODO(https://github.com/brave/brave-browser/issues/12345): Remove
// this workaround once upstream fixes the validation race condition.
if (ShouldSkipValidation()) return;

// FooApi::Connect() relies on the receiver sending Ack messages, but the JS
// bindings don't implement this protocol. Use an explicit timer instead.
base::OneShotTimer idle_timer_;
```
