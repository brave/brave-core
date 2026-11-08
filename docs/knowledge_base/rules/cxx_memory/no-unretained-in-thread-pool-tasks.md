---
name: brave/no-unretained-in-thread-pool-tasks
description:
  'Tasks posted to the thread pool must never bind base::Unretained; run free
  functions there and reply to the main thread with a WeakPtr.'
rubric: safety
severity: blocking
---

**Rule.** NEVER bind `base::Unretained(...)` into a task posted to the thread
pool (`base::ThreadPool::PostTask*()`, or a task runner it created). Run the
OS-specific or blocking work as a static or free function on the pool, and
handle its result on the main thread through `PostTaskAndReplyWithResult()` with
a reply bound to a `WeakPtr`.

**Why.** The object can be destroyed on its own sequence while the pool task is
queued or running, and nothing cancels the task, so `Unretained` across threads
leads to use-after-free.

## Examples

Bad:

```cpp
base::ThreadPool::PostTask(
    FROM_HERE, base::BindOnce(&MyClass::DoWork, base::Unretained(this)));
```

Good:

```cpp
base::ThreadPool::PostTaskAndReplyWithResult(
    FROM_HERE, base::BindOnce(&DoBlockingWork),
    base::BindOnce(&MyClass::OnWorkDone, weak_factory_.GetWeakPtr()));
```
