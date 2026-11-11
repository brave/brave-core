---
name: android_ios/java-campaigns
rubric: android_ios
severity: migration
---

**Rule.** Brave's Java follows upstream's Java campaigns: NullAway
`@NullMarked`, the new supplier types, and ErrorProne checks such as
`DoNotMock`.

**Why.** Each campaign ends in enforcement, and Brave code still on the old
idiom then fails the build, or carries suppressions that hide real issues.

**How to check.** The candidate commits advance a campaign. Count Brave's
remaining old-idiom uses and suppressions for each, and grep `@NullMarked` files
for androidx nullness annotations, which can't annotate type arguments.
