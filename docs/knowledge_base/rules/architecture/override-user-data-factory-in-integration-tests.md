---
name: brave/override-user-data-factory-in-integration-tests
description:
  "Integration tests substitute a mock tab or window feature with
  GetUserDataFactoryForTesting().AddOverrideForTesting() before the browser
  starts; unit tests attach the feature to a mock interface's
  UnownedUserDataHost."
rubric: testing
severity: should_fix
---

**Rule.** In a browser test that needs a mock of a tab or window feature,
install it with `GetUserDataFactoryForTesting().AddOverrideForTesting()` before
the browser is created, for example in the fixture's constructor or `SetUp()`.
In a unit test, give a mock `TabInterface` (or `BrowserWindowInterface`) a
`ui::UnownedUserDataHost`, and construct the feature against it.

**Why.** The factory override makes the browser create the mock in place of the
real feature, with the real browser infrastructure around it; the unit-test host
lets the feature register itself as in production, without
`BrowserWithTestWindowTest`.

## Examples

Bad:

```cpp
void SetUpOnMainThread() override {
  // Too late: the real feature already exists for the first tab.
  auto override = tabs::TabFeatures::GetUserDataFactoryForTesting()
      .AddOverrideForTesting(base::BindRepeating(...));
}
```

Good:

```cpp
FooBrowserTest() {
  override_ = tabs::TabFeatures::GetUserDataFactoryForTesting()
      .AddOverrideForTesting(base::BindRepeating([](tabs::TabInterface& tab) {
        return std::make_unique<MyTabFeatureMock>(&tab);
      }));
}
```
