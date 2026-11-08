---
name: brave/evaljs-over-execute-script-and-extract
description:
  'Browser tests run script with content::EvalJs and content::ExecJs, never the
  deprecated ExecuteScriptAndExtract* functions.'
rubric: testing
severity: should_fix
---

**Rule.** Use `content::EvalJs()` to get a value from script, and
`content::ExecJs()` to run script for its effect. Do not use
`ExecuteScriptAndExtractBool/String/Int()` or
`window.domAutomationController.send()`.

**Why.** The `ExecuteScriptAndExtract*` functions are deprecated: they need the
script to call `domAutomationController.send()`, hang when it does not, and give
poor failure messages. `EvalJs()` returns the script's value, waits for
promises, and reports script errors.

## Examples

Bad:

```cpp
bool result;
ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
    web_contents, "domAutomationController.send(someCheck())", &result));
```

Good:

```cpp
EXPECT_EQ(true, content::EvalJs(web_contents, "someCheck()"));
```
