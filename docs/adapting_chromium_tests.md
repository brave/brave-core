# Adapting Chromium tests for the Brave codebase

To make Chromium tests work properly in the Brave codebase, you may need to
modify either the test logic or the production logic. This document explains the
available mechanisms in the `check_is_test.h` and `chrome_test_suite.cc` files
to accomplish this.

## Mechanisms for adapting tests

### 1. Checking `CurrentTestVendor` in production code

The `CurrentTestVendor::Get()` function allows you to determine the currently
running test vendor. It is set to `kBrave` for `brave_*` test binaries and
`kChromium` for Chromium test binaries. This can be used to conditionally alter
production logic during tests.

#### Example usage

```cpp
#include "base/check_is_test.h"

void SomeFunction() {
  if (base::CurrentTestVendor::Get() == base::TestVendor::kChromium) {
    // Alter logic for Chromium tests.
  } else {
    // Default Brave logic.
  }
}
```

### 2. Customizing active features in a test by pattern

The `BraveChromeTestSetupHelper` class in
`//brave/chromium_src/chrome/test/base/chrome_test_suite.cc` allows you to
customize test behavior by enabling or disabling features based on test
patterns.

Note: This approach should only be used for Chromium tests to avoid unnecessary
patching of the upstream source files.

#### Example usage

The `BraveChromeTestSetupHelper` class uses a list of `TestAdjustments` to
specify which features to enable or disable for specific test patterns.

```cpp
class BraveChromeTestSetupHelper : public testing::EmptyTestEventListener {
  void OnTestStart(const testing::TestInfo& test_info) override {
    static const base::NoDestructor<std::vector<TestAdjustments>> kTestAdjustments({
        {
            .test_patterns = {"SomeTestPattern.*"},
            .enable_features = {"SomeFeature"},
            .disable_features = {"AnotherFeature"},
        },
    });
    // ...
  }
};
```
