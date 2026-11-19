---
name: brave/prefer-fakes-over-real-dependencies-in-tests
description:
  A unit test gives the code under test fakes or mocks it controls for its
  dependencies, not the real implementations.
rubric: testing
severity: should_fix
---

**Rule.** In a unit test, replace the dependencies of the class under test with
fakes (or mocks) that the test controls, such as a `MockTool` whose completion
callback the test runs when it chooses, rather than constructing the real
implementation (the real navigation tool, a real service fetched from its
`KeyedService` factory). Flag a unit test that builds a real dependency whose
behaviour or timing the test then has to work around.

**Why.** A real dependency couples the test to code it does not test: a bug in
that dependency fails this test as well, and its own timing makes the test
slower and less deterministic. A fake lets the test decide exactly when and how
the dependency answers, which also keeps the production code modular.

## Examples

Bad:

```cpp
// The real tool navigates, so the test cannot choose when it completes.
auto tool = std::make_unique<NavigationTool>(web_contents());
ClickPauseButton();
```

Good:

```cpp
auto mock_tool = std::make_unique<MockTool>();
EXPECT_CALL(*mock_tool, Execute(_, _))
    .WillOnce([&](auto data, auto callback) {
      tool_callback = std::move(callback);
    });

ClickPauseButton();
std::move(tool_callback).Run(/* result */);
```
