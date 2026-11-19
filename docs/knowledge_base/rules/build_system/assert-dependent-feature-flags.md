---
name: brave/assert-dependent-feature-flags
description:
  "When feature A depends on feature B, A's BUILD.gn asserts B's build flag
  instead of making A work without B."
rubric: design
severity: should_fix
---

**Rule.** When feature A needs feature B, add an `assert()` in GN that B's build
flag is true when A is enabled, with a message naming the requirement. Do not
add code paths, guards or fallbacks that make A work without B for
configurations that are not supported.

**Why.** The assert turns an unsupported flag combination into an immediate
`gn gen` error with an actionable message. Code that tolerates the missing
feature is untested, and grows guards around every use of B.

## Examples

Bad:

```gn
# brave/components/brave_rewards/BUILD.gn
if (enable_brave_wallet) {
  deps += [ "//brave/components/brave_wallet/browser" ]
}
```

Good:

```gn
# brave/components/brave_rewards/BUILD.gn
assert(enable_brave_wallet,
       "Rewards requires Wallet (enable_brave_wallet=true)")
```
