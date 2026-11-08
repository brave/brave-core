---
name: brave/ios-dependency-injection-over-singletons
description:
  'iOS code receives services through initializers, method parameters or the
  Tab/TabHelper extension pattern, not through singletons.'
rubric: design
severity: should_fix
---

**Rule.** Do not add a singleton (`static let shared`, `+sharedInstance`) for a
service, and do not reach a service through one (`Foo.shared`) in new code. Pass
the service in through the initializer or a method parameter, or attach it to
the tab through the `Tab`/`TabHelper` extension pattern.

**Why.** A singleton hides the dependency, shares state across profiles and
tests, and cannot be replaced by a fake; an injected dependency is explicit and
testable.

## Examples

Bad:

```swift
let controller = BraveRewardsViewController.shared
controller.fetchBalance()
```

Good:

```swift
init(rewardsController: BraveRewardsViewController) {
  self.rewardsController = rewardsController
}
```
