---
name: android_ios/ios-protocols-brave-adopts
rubric: android_ios
severity: regression
---

**Rule.** Brave's Objective-C bridges and Swift types implement every method of
the upstream protocols they adopt.

**Why.** Upstream's protocols have no `@optional` methods, so a new method Brave
lacks fails the iOS build, or the Swift conformance.

**How to check.** For each protocol a candidate commit changes, diff
`git show <tag>:<header> | grep '^- ('` at both tags, and check Brave's
conformances, including Swift ones in `ios/brave-ios`
(`git grep -nE 'class \w+: .*\b<Protocol>\b' -- '*.swift'`).

## Examples

- M157, 8103.1: `CWVAutofillClientIOSBridge` gained four methods that
  `BraveAutofillClientBridge` lacks.
- M158, 8113.1: Swift's `MockCredential` missed two `Credential` properties.
