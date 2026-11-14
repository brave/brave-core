---
name: brave/explicit-task-traits-for-thread-pool-tasks
description:
  'Thread pool tasks explicitly specify base::TaskPriority and shutdown behavior
  instead of relying on the defaults.'
rubric: perf
severity: should_fix
---

**Rule.** When posting a task to the thread pool or creating a thread pool task
runner, spell out a `base::TaskPriority` and a `base::TaskShutdownBehavior` in
its traits. Use `BEST_EFFORT` for non-urgent work and `SKIP_ON_SHUTDOWN` when
the work can safely be abandoned.

**Why.** By default a task runs at `USER_BLOCKING` priority, so non-urgent work
competes with the work the user is waiting for, and its shutdown behavior is
left to the task's recipient. Explicit traits state what the work needs.

## Examples

Bad:

```cpp
base::ThreadPool::PostTask(FROM_HERE, {base::MayBlock()}, task);
```

Good:

```cpp
base::ThreadPool::PostTask(
    FROM_HERE,
    {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
     base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
    task);
```
