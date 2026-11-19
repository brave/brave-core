---
name: brave/relocate-test-checks-when-refactoring
description:
  A refactoring that removes a test check moves it to where the behaviour now
  lives, or documents why it no longer applies; coverage never drops.
rubric: testing
severity: blocking
---

**Rule.** When a change deletes an assertion, an `EXPECT_CALL` or a whole test
as part of a refactoring, the same check MUST reappear where the behaviour now
lives (another test, another fixture, the new class's tests). If the check no
longer applies, the change says why, in the test or the commit message. Flag a
removed check with no counterpart and no explanation.

**Why.** Refactorings are where regressions slip in, and a check deleted along
the way silently drops the coverage that would have caught them.

## Examples

Bad:

```diff
 TEST_F(FooServiceTest, Fetch) {
   service_->Fetch();
-  EXPECT_EQ(service_->cache_size(), 1u);
 }
```

Good:

```diff
 TEST_F(FooServiceTest, Fetch) {
   service_->Fetch();
-  EXPECT_EQ(service_->cache_size(), 1u);
 }
+
+TEST_F(FooCacheTest, StoresFetchedEntry) {
+  cache_.Store(MakeEntry());
+  EXPECT_EQ(cache_.size(), 1u);
+}
```
