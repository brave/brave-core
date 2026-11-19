---
name: brave/objc-pass-weak-self-into-posted-tasks
description:
  'Tasks posted from Objective-C++ receive weakSelf as a parameter and use it as
  a strong reference, instead of a block that uses a captured weakSelf several
  times.'
rubric: safety
severity: should_fix
---

**Rule.** When a task posted from Objective-C++ needs `self`, bind `weakSelf` as
an argument of the callback
(`base::BindOnce([](__typeof(self) strongSelf) { ... }, weakSelf)`), and return
early when `strongSelf` is `nil` before using it. Do not capture `weakSelf` in a
block that uses it more than once. A block whose only statement is one message
to `weakSelf` can capture it directly; NEVER flag that case.

**Why.** Each use of a captured `weakSelf` reads the weak reference again, so
`self` can be deallocated between two uses and the task runs half its work. The
bound parameter holds a strong reference for the whole task, or `nil` from its
start. A single message to `nil` is a no-op, so a one-call block is safe.

## Examples

Bad:

```objc
__weak __typeof(self) weakSelf = self;
task_runner->PostTask(FROM_HERE, base::BindOnce(^{
  [weakSelf prepareWork];
  [weakSelf doWork];  // weakSelf may have become nil after the first call.
}));
```

Good:

```objc
__weak __typeof(self) weakSelf = self;
task_runner->PostTask(
    FROM_HERE, base::BindOnce(
                   [](__typeof(self) strongSelf) {
                     if (!strongSelf) {
                       return;
                     }
                     [strongSelf prepareWork];
                     [strongSelf doWork];
                   },
                   weakSelf));

// One message to weakSelf: capturing it directly is fine.
task_runner->PostTask(FROM_HERE, base::BindOnce(^{
  [weakSelf doWork];
}));
```
