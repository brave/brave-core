# Test Isolation and Specific Patterns

<a id="TI-001"></a>

## Test in Isolation with Fakes

**Prefer fakes over real dependencies:**
- Prevents cascading test failures
- Produces more maintainable, modular code
- Makes tests faster and more deterministic

**Example: Use MockTool instead of real navigation tool:**
```cpp
// ✅ GOOD - Control exact timing with MockTool
auto mock_tool = std::make_unique<MockTool>();
EXPECT_CALL(*mock_tool, Execute(_, _))
    .WillOnce([&](auto data, auto callback) {
      tool_callback = std::move(callback);
    });

// Now we can control exactly when the tool completes
ClickPauseButton();
std::move(tool_callback).Run(/* result */);
```

---

<a id="TI-002"></a>

## Test the API, Not Implementation

**Focus on public interfaces:**
- Allows internal implementation changes without breaking tests
- Provides accurate usage examples for other developers
- Makes tests more maintainable

**BAD:**
```cpp
// ❌ Testing internal implementation details
EXPECT_EQ(object->private_state_, 42);
```

**GOOD:**
```cpp
// ✅ Testing public API behavior
EXPECT_TRUE(object->IsReady());
EXPECT_EQ(object->GetValue(), 42);
```

---

<a id="TI-003"></a>

## HTTP Request Testing - Per-Domain Expected Values

When testing HTTP headers from pages with subresources, use per-domain maps:

**BAD:**
```cpp
// ❌ WRONG - Global expected value, race condition with subresources
std::string expected_header_;

void SetExpectedHeader(const std::string& value) {
  expected_header_ = value;
}

void OnRequest(const HttpRequest& request) {
  EXPECT_EQ(request.headers["Accept-Language"], expected_header_);
}
```

**GOOD:**
```cpp
// ✅ CORRECT - Per-domain expected values
std::map<std::string, std::string> expected_headers_;

void SetExpectedHeader(const std::string& domain, const std::string& value) {
  expected_headers_[domain] = value;
}

void OnRequest(const HttpRequest& request) {
  std::string domain = ExtractDomain(request.headers["Host"]);
  EXPECT_EQ(request.headers["Accept-Language"], expected_headers_[domain]);
}
```

Subresource requests can arrive out-of-order, so per-resource expected values prevent race conditions.

---

<a id="TI-004"></a>

## Throttle Testing - Use Large Throttle Windows

**Problem:** Throttle timers start when a fetch happens, not when test timing checks begin.

**BAD:**
```cpp
// ❌ WRONG - 500ms throttle, but WaitForSelector might take 250ms!
const int kThrottleMs = 500;
WaitForSelectorBlocked();  // May take 200-300ms
NonBlockingDelay(base::Milliseconds(250));  // Checking too early!
```

**GOOD:**
```cpp
// ✅ CORRECT - Large throttle window accounts for polling time
const int kThrottleMs = 2000;
WaitForSelectorBlocked();  // Takes ~200-300ms
NonBlockingDelay(base::Milliseconds(1000));  // Still well within throttle
```

Polling-based waits consume time from the throttle window, so use throttle times much larger than expected polling durations.

---

<a id="TI-005"></a>

## Chromium Pattern Research

<a id="TI-006"></a>

### ✅ Search for Existing Chromium Patterns

**Before implementing a fix for async/timing issues, search the Chromium codebase for similar patterns.**

When you encounter a testing problem (flakiness, timing issues, async operations), ask:
1. Does Chromium have tests with similar requirements?
2. What patterns do they use to solve this?
3. Is there a more deterministic, event-driven approach?

**Research workflow:**
1. Generate an initial fix proposal
2. Search Chromium codebase for similar test scenarios
3. Compare your approach to existing Chromium patterns
4. Prefer established Chromium patterns over novel solutions

<a id="TI-007"></a>

### ✅ Include Chromium Code References

**When following a Chromium pattern, include a reference in your code comments.**

**GOOD - Reference in code comment:**
```cpp
// NOTE: Replace() is an IPC to the renderer that updates the DOM
// asynchronously. We use a MutationObserver to wait for the DOM to update
// to the expected value before checking.
// Pattern from service_worker_internals_ui_browsertest.cc.
static constexpr char kWaitForTextScript[] = R"(
  // ...
)";
```

**GOOD - Reference in commit message:**
```
Fix flaky RewriteInPlace_ContentEditable test

The MutationObserver pattern follows the approach used in Chromium's
service_worker_internals_ui_browsertest.cc.
```

---

<a id="TI-008"></a>

## ✅ Always Check `EvalJs` Results

**When using `EvalJs` in tests, always check the result with `EXPECT_TRUE`/`EXPECT_EQ`.** An unchecked `EvalJs` call silently swallows errors - any gibberish expression would appear to "pass" the test.

```cpp
// ❌ WRONG - unchecked EvalJs, silently ignores exceptions
content::EvalJs(web_contents, "window.solana.isConnected");

// ✅ CORRECT - result checked
EXPECT_EQ(true, content::EvalJs(web_contents,
                                "window.solana.isConnected"));
```

---

<a id="TI-009"></a>

## ❌ Don't Duplicate Test Constants - Expose via Header

**Don't duplicate constants between production code and test code.** If both need the same value, expose it via a shared header file.

```cpp
// ❌ WRONG - same constant duplicated in test
// production.cc
constexpr base::TimeDelta kCleanupDelay = base::Seconds(30);
// test.cc
constexpr base::TimeDelta kCleanupDelay = base::Seconds(30);  // duplicated!

// ✅ CORRECT - shared via header
// constants.h
inline constexpr base::TimeDelta kCleanupDelay = base::Seconds(30);
```

---

<a id="TI-010"></a>

## ❌ Don't Depend on `//chrome` from Components Tests

**Component-level unit tests must not depend on `//chrome`.** If you need objects typically created by Chrome infrastructure, create them manually in the test.

```cpp
// ❌ WRONG - depending on //chrome from components test
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
auto* settings_map = HostContentSettingsMapFactory::GetForProfile(profile);

// ✅ CORRECT - create the instance directly
auto settings_map = base::MakeRefCounted<HostContentSettingsMap>(
    &pref_service_, false /* is_off_the_record */,
    false /* store_last_modified */, false /* restore_session*/,
    false /* should_record_metrics */);
```

---

<a id="TI-011"></a>

## ✅ Prefer `*_for_testing()` Accessors Over `FRIEND_TEST`

**When tests need access to a single private member, provide a `*_for_testing()` accessor** returning a reference instead of adding multiple `FRIEND_TEST` macros.

```cpp
// ❌ WRONG - proliferating FRIEND_TEST macros
class BraveTab {
  FRIEND_TEST_ALL_PREFIXES(BraveTabTest, RenameBasic);
  FRIEND_TEST_ALL_PREFIXES(BraveTabTest, RenameCancel);
  FRIEND_TEST_ALL_PREFIXES(BraveTabTest, RenameSubmit);
  raw_ptr<views::Textfield> rename_textfield_;
};

// ✅ CORRECT - single accessor
class BraveTab {
  views::Textfield& rename_textfield_for_testing() { return *rename_textfield_; }
  raw_ptr<views::Textfield> rename_textfield_;
};
```

---

<a id="TI-012"></a>

## ✅ Verify Tests Actually Test What They Claim

**When using test-only controls (globals, mocks, feature overrides), verify the test still exercises the code path it claims to test.** A test that disables the code under test proves nothing.

```cpp
// ❌ WRONG - test disables the code it claims to test
void SetUp() { g_disable_stats_for_testing = true; }
// "The stats updater should not reach the endpoint"
// But we disabled stats entirely - this test proves nothing!

// ✅ CORRECT - test the actual behavior with a mock server
```

---

<a id="TI-013"></a>

## ✅ Re-enable Test-Only Globals in `TearDown`

**When a test disables functionality via a global in `SetUp()`, always re-enable it in `TearDown()`** to avoid leaking state to other tests.

```cpp
// ❌ WRONG - leaks state to other tests
void SetUp() { g_disable_auto_start = true; }

// ✅ CORRECT - restore state
void SetUp() { g_disable_auto_start = true; }
void TearDown() { g_disable_auto_start = false; }
```

---

<a id="TI-014"></a>

## ✅ Include Diagnostic State in Test Assertions

**Test assertions should include diagnostic context using the `<<` operator** so failures are debuggable without re-running under a debugger.

```cpp
// ❌ WRONG - no context on failure
EXPECT_EQ(result.status, Status::kSuccess);

// ✅ CORRECT - diagnostic context
EXPECT_EQ(result.status, Status::kSuccess)
    << "Failed for URL: " << url << ", response code: " << result.code;
```

---

<a id="TI-015"></a>

## ✅ Use `SCOPED_TRACE` with `base::Location` in Browser Test Helpers

**Use `SCOPED_TRACE(base::Location::Current())` in browser test helper functions** to get file/line information in failure output. Without this, failures in helpers only show the helper's line, not the caller.

```cpp
// ❌ WRONG - failure points to helper, not caller
void VerifyTabCount(Browser* browser, int expected) {
  EXPECT_EQ(browser->tab_strip_model()->count(), expected);
}

// ✅ CORRECT - failure shows caller location
void VerifyTabCount(Browser* browser, int expected,
                    base::Location location = base::Location::Current()) {
  SCOPED_TRACE(location.ToString());
  EXPECT_EQ(browser->tab_strip_model()->count(), expected);
}
```

---

<a id="TI-016"></a>

## ✅ Unit Tests Required for Bad/Invalid Input

**Tool and handler implementations must have unit tests covering bad/invalid input scenarios,** not just happy paths. Test with malformed data, empty inputs, null values, and out-of-range parameters.

---

<a id="TI-017"></a>

## ✅ Relocate Test Checks When Refactoring

**When refactoring removes a test check, that check must be relocated to the appropriate new location, not simply deleted.** Test coverage should never decrease during refactoring. If a check is no longer applicable, document why.

---

<a id="TI-018"></a>

## ✅ Verify Ordering in Tests That Return Ordered Data

**When testing functions that return ordered data, explicitly verify the order of results in the test,** not just the content. If the function is supposed to return results in a specific order (e.g., most recent first), the test should assert that order.

```cpp
// ❌ WRONG - only checks content exists
EXPECT_EQ(results.size(), 3u);
EXPECT_TRUE(ContainsUrl(results, "https://a.com"));
EXPECT_TRUE(ContainsUrl(results, "https://b.com"));

// ✅ CORRECT - verifies order
EXPECT_EQ(results.size(), 3u);
EXPECT_EQ(results[0].url, "https://c.com");  // most recent
EXPECT_EQ(results[1].url, "https://b.com");
EXPECT_EQ(results[2].url, "https://a.com");  // oldest
```

---

<a id="TI-020"></a>

## ✅ Use `base::test::ParseJsonDict` for Test Comparisons

**In tests comparing JSON values, use `base::test::ParseJsonDict()`** for simpler, more readable assertions. **Never use `testing::HasSubstr`** to validate JSON -- it is fragile and doesn't verify structure.

```cpp
// ❌ WRONG - substring matching on JSON
EXPECT_THAT(json_str, testing::HasSubstr("\"role\":\"user\""));

// ❌ WRONG - manually building expected dict
base::Value::Dict expected;
expected.Set("method", "chain_getBlockHash");
expected.Set("id", 1);
EXPECT_EQ(actual, expected);

// ✅ CORRECT - parse from string
EXPECT_EQ(actual, base::test::ParseJsonDict(
    R"({"method":"chain_getBlockHash","id":1})"));
```

---

<a id="TI-021"></a>

## ✅ Verify Negative Expectations in Tests

**When testing error/failure paths, verify that side effects that should NOT occur are explicitly checked.** Don't only test the positive path.

```cpp
// ❌ WRONG - only checks the error is returned, doesn't verify no side effects
EXPECT_FALSE(result.has_value());

// ✅ CORRECT - also verify no unwanted side effects
EXPECT_CALL(observer, OnConversationTitleChanged(_)).Times(0);
EXPECT_FALSE(result.has_value());
```

---

<a id="TI-022"></a>

## ✅ Test All Fields in Serialization Tests

**Verify ALL fields in serialization/deserialization tests, including secondary fields like favicon URLs.** Also always test deserialization of invalid/malformed input.

```cpp
// ❌ WRONG - only checks primary fields
EXPECT_EQ(result.title, "Test");
EXPECT_EQ(result.url, "https://example.com");
// Missing: favicon_url, description, etc.

// ✅ CORRECT - verify all fields
EXPECT_EQ(result.title, "Test");
EXPECT_EQ(result.url, "https://example.com");
EXPECT_EQ(result.favicon_url, "https://example.com/favicon.ico");

// ✅ ALSO CORRECT - test invalid input
auto bad_result = Deserialize("{\"url\": \"not-a-valid-url\"}");
EXPECT_FALSE(bad_result.has_value());
```

---

<a id="TI-023"></a>

## ✅ Use Parameterized Tests for Similar Scenarios

**When testing multiple similar scenarios with different inputs/outputs, use parameterized tests instead of separate test functions.**

```cpp
// ❌ WRONG - separate tests for each variation
TEST_F(MyTest, HandleTypeA) { ... }
TEST_F(MyTest, HandleTypeB) { ... }
TEST_F(MyTest, HandleTypeC) { ... }

// ✅ CORRECT - parameterized with SCOPED_TRACE
struct TestCase {
  std::string name;
  ActionType type;
  std::string expected;
};

TEST_F(MyTest, HandleAllTypes) {
  const TestCase cases[] = {
    {"TypeA", ActionType::kA, "expected_a"},
    {"TypeB", ActionType::kB, "expected_b"},
    {"TypeC", ActionType::kC, "expected_c"},
  };
  for (const auto& tc : cases) {
    SCOPED_TRACE(tc.name);
    EXPECT_EQ(Handle(tc.type), tc.expected);
  }
}

// ✅ ALSO CORRECT - INSTANTIATE_TEST_SUITE_P for larger test suites
```

---

<a id="TI-024"></a>

## ✅ Extract Repetitive Test Assertions into Helpers

**When tests repeat the same assertion blocks (e.g., checking content block types and fields), extract into reusable helper functions.**

```cpp
// ❌ WRONG - repeated assertion blocks
EXPECT_TRUE(blocks[0]->is_text_content_block());
EXPECT_EQ(blocks[0]->get_text_content_block()->text, "hello");
EXPECT_TRUE(blocks[1]->is_image_content_block());
EXPECT_EQ(blocks[1]->get_image_content_block()->url, "img.png");

// ✅ CORRECT - reusable helpers
void VerifyTextBlock(const ContentBlock& block, const std::string& text) {
  ASSERT_TRUE(block.is_text_content_block());
  EXPECT_EQ(block.get_text_content_block()->text, text);
}

VerifyTextBlock(*blocks[0], "hello");
VerifyImageBlock(*blocks[1], "img.png");
```

---

<a id="TI-025"></a>

## ✅ Use `ASSERT_TRUE` Before Accessing Optional Values

**In tests, use `ASSERT_TRUE(optional.has_value())` before accessing an optional's value.** Do not use `value_or("")` as a fallback -- it hides bugs.

```cpp
// ❌ WRONG - hides missing values
EXPECT_EQ(map[entry->uuid.value_or("")], expected_content);

// ✅ CORRECT - fail fast on missing value
ASSERT_TRUE(entry->uuid.has_value());
EXPECT_EQ(map[*entry->uuid], expected_content);
```

---

<a id="TI-026"></a>

## ✅ Test Both Sides of Guard Conditions

**When testing code with a guard condition (e.g., blocked during loading, enabled after), test both sides:** verify the behavior is blocked when the guard is active AND that it proceeds correctly when the guard is released.

```cpp
// ❌ WRONG - only tests the happy path
TEST_F(MyTest, ActionSucceeds) {
  SetupCompleteState();
  EXPECT_TRUE(DoAction());
}

// ✅ CORRECT - tests both sides
TEST_F(MyTest, ActionBlockedDuringLoad) {
  EXPECT_FALSE(DoAction());  // Blocked while loading
}
TEST_F(MyTest, ActionSucceedsAfterLoad) {
  CompleteLoading();
  EXPECT_TRUE(DoAction());  // Proceeds after load
}
```

---

<a id="TI-027"></a>

## ❌ Don't Simulate Impossible Production States in Tests

**Tests should not construct states that cannot occur in production.** If a test needs to set up a scenario, it should follow the same code path as production rather than directly manipulating internal state into an impossible configuration. Tests for impossible states prove nothing and can give false confidence.

---

<a id="TI-028"></a>

## ✅ Use `WillOnce` Instead of `WillRepeatedly` for Single Calls

**When a mock method is expected to be called exactly once, use `WillOnce` instead of `WillRepeatedly`.** `WillRepeatedly` hides bugs where the method is called more times than expected.

```cpp
// ❌ WRONG - hides extra calls
EXPECT_CALL(mock, Fetch(_)).WillRepeatedly(Return(result));

// ✅ CORRECT - fails if called more than once
EXPECT_CALL(mock, Fetch(_)).WillOnce(Return(result));
```

---

<a id="TI-029"></a>

## ✅ Verify Observable Side Effects in Browser Tests

**Browser tests should verify observable side effects** (e.g., a tab actually appeared, a URL actually loaded, a dialog was shown) rather than just checking return values. This catches integration issues that unit tests miss.

```cpp
// ❌ WRONG - only checks return value
EXPECT_TRUE(OpenNewTab(url));

// ✅ CORRECT - verifies the tab actually appeared
EXPECT_TRUE(OpenNewTab(url));
EXPECT_EQ(browser()->tab_strip_model()->count(), initial_count + 1);
EXPECT_EQ(GetActiveWebContents()->GetURL(), url);
```

---

<a id="TI-030"></a>

## ✅ Bug-Fix PRs Must Include Test Coverage

**Every bug-fix PR should include a test that reproduces the specific bug scenario being fixed.** Without this, there's no guarantee the bug won't regress. The test should fail without the fix and pass with it.

---

<a id="TI-033"></a>

## ✅ When Fixing a Test, Run All Tests in the Same File

**When fixing a flaky or failing test, run all tests in the same file across all test fixtures,** not just the single test being changed. This catches regressions or interactions between tests that share fixtures.

---

<a id="TI-034"></a>

## ✅ Use ASSERT for Preconditions That Subsequent Code Depends On

**When a test check guards state that later code depends on, use `ASSERT_*` (fatal) instead of `EXPECT_*` (non-fatal).** A failing `EXPECT` allows continued execution, which can crash on null dereferences or produce confusing secondary failures.

```cpp
// ❌ WRONG - EXPECT allows crash on null dereference
EXPECT_TRUE(item != nullptr);
item->DoSomething();  // crashes if item is null!

// ✅ CORRECT - ASSERT stops the test on failure
ASSERT_TRUE(item != nullptr);
item->DoSomething();  // only reached if item is valid
```

---

<a id="TI-035"></a>

## ✅ Use `DETACH_FROM_SEQUENCE()` in Database Constructors

**Database classes that use `SEQUENCE_CHECKER` should call `DETACH_FROM_SEQUENCE(sequence_checker_)` in their constructor.** This allows the sequence checker to re-bind to whatever sequence first uses the object, which is essential for mock-based testing where objects may be created on a different sequence than they're used on.

```cpp
// ❌ WRONG - sequence checker binds to construction thread
MyDatabase::MyDatabase() {
  // sequence_checker_ bound to current thread
}

// ✅ CORRECT - detach so it re-binds on first use
MyDatabase::MyDatabase() {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}
```

---

<a id="TI-036"></a>

## ✅ Use `TEST()` Instead of `TEST_F()` for Empty Fixtures

**When a test fixture class is empty (inherits from `testing::Test` with no added members, `SetUp()`, or `TearDown()`), use the plain `TEST()` macro.** `TEST_F` is only needed when the fixture provides shared state or configuration across tests.

```cpp
// ❌ WRONG - empty fixture with TEST_F
class MyUtilsTest : public testing::Test {};

TEST_F(MyUtilsTest, ParsesValidInput) {
  EXPECT_EQ(Parse("abc"), "abc");
}

// ✅ CORRECT - no fixture needed
TEST(MyUtilsTest, ParsesValidInput) {
  EXPECT_EQ(Parse("abc"), "abc");
}
```

---

<a id="TI-037"></a>

## ✅ Use `NOTREACHED()` with Request Body in Test Mock Server Handlers

**In test mock servers and URL interceptors, use `NOTREACHED() << request.content` (or equivalent) as the default handler to catch unhandled requests.** This makes test failures immediately obvious and includes the request body in the failure message for debugging.

```cpp
// ❌ WRONG - silently returns empty response for unhandled requests
std::unique_ptr<HttpResponse> HandleRequest(const HttpRequest& request) {
  if (request.relative_url == "/api/v1") { return MakeResponse(); }
  return nullptr;  // silent failure
}

// ✅ CORRECT - fail loudly on unhandled requests
std::unique_ptr<HttpResponse> HandleRequest(const HttpRequest& request) {
  if (request.relative_url == "/api/v1") { return MakeResponse(); }
  NOTREACHED() << "Unhandled request: " << request.relative_url
               << " body: " << request.content;
}
```

---

<a id="TI-038"></a>

## ✅ Use Descriptive Test Method Names

**Test method names should be long and descriptive enough to explain the scenario without reading the test body.** Do not sacrifice clarity to save characters. Long names are preferable to short, cryptic names.

```cpp
// ❌ WRONG - too terse
TEST_F(WalletServiceTest, TestSend) { ... }
TEST_F(WalletServiceTest, Error) { ... }

// ✅ CORRECT - self-documenting
TEST_F(WalletServiceTest, SendTransaction_ReturnsErrorForInsufficientBalance) { ... }
TEST_F(WalletServiceTest, GetTokenBalances_HandlesNetworkTimeoutGracefully) { ... }
```

---

<a id="TI-039"></a>

## ✅ PRs Should Include Reasonable Test Coverage

**Code that can be tested should be tested.** When reviewing or writing a PR, ensure that new logic, branches, and edge cases have corresponding tests. This applies to all PRs, not just bug fixes. Don't be overly strict — use judgment — but flag obvious gaps where meaningful test coverage is missing.

**If code is difficult to test, suggest refactors that improve testability** rather than accepting untested code. Common refactors include extracting logic into pure functions, adding dependency injection, or separating I/O from computation.

```cpp
// ❌ WRONG - new feature PR with no tests for core logic
// "We'll add tests later" or "it's too hard to test"

// ✅ CORRECT - PR includes tests for the new behavior
// If the code is hard to test, refactor to make it testable:
//   - Extract business logic into a standalone function
//   - Use dependency injection for external dependencies
//   - Separate pure computation from side effects
```

---

<a id="TI-040"></a>

## ✅ Use chromium_src Include Pattern to Test Upstream Overrides

**When overriding upstream Chromium code via `chromium_src`, include the upstream test file and add Brave-specific tests after it.** This runs all upstream tests (catching regressions from your overrides) plus your new tests, with minimal effort.

```cpp
// chromium_src/components/foo/foo_service_unittest.cc

// Include the entire upstream test file — all its tests will run.
#include <components/foo/foo_service_unittest.cc>

namespace foo {

// Add Brave-specific tests using the upstream fixtures and mocks.
TEST_F(FooServiceTest, BraveSpecificBehavior) {
  // Tests Brave-specific logic that was added via chromium_src override
}

}  // namespace foo
```

Good examples in the repo:
- `chromium_src/components/sync/engine/sync_scheduler_impl_unittest.cc`
- `chromium_src/components/ntp_tiles/most_visited_sites_unittest.cc`
- `chromium_src/components/variations/service/variations_service_unittest.cc`

