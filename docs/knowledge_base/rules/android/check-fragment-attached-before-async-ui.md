---
name: brave/check-fragment-attached-before-async-ui
description:
  An async callback that updates UI through a Fragment must first return unless
  the fragment is still added and its host Activity exists and is not finishing.
rubric: correctness
severity: blocking
---

**Rule.** When a service callback, a `PostTask.postTask()` task, a Mojo response
or any other asynchronous path updates UI through a Fragment, it MUST first
return if `!isAdded() || isDetached()`, and if `getActivity()` is null or
`isFinishing()`. NEVER assume the fragment that started the work is still
attached when the result arrives.

**Why.** A Fragment can be detached, or its Activity destroyed, while the async
work is in progress. The callback then dereferences a null `getActivity()` or
updates views that are gone.

## Examples

Bad:

```java
void onServiceResult(Result result) {
    updateUI(result);
}
```

Good:

```java
void onServiceResult(Result result) {
    if (!isAdded() || isDetached()) return;
    Activity activity = getActivity();
    if (activity == null || activity.isFinishing()) return;
    updateUI(result);
}
```
