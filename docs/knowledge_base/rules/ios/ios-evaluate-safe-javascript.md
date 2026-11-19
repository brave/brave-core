---
name: brave/ios-evaluate-safe-javascript
description:
  'Swift code evaluates JavaScript through the evaluateSafeJavaScript wrapper,
  not by calling callAsyncJavaScript or evaluateJavaScript directly.'
rubric: security
severity: should_fix
---

**Rule.** Run JavaScript in a web view through the `evaluateSafeJavaScript`
wrapper. Do not call `callAsyncJavaScript` or `evaluateJavaScript` directly.

**Why.** The wrapper provides the error handling and the security checks that a
direct call skips.

## Examples

Bad:

```swift
let result = try await webView.callAsyncJavaScript(
  "return cr.googleTranslate.sourceLang", arguments: [:], in: nil,
  contentWorld: BraveTranslateScriptHandler.scriptSandbox)
```

Good:

```swift
let (result, error) = await tab.evaluateSafeJavaScript(
  functionName: "cr.googleTranslate.sourceLang",
  contentWorld: BraveTranslateScriptHandler.scriptSandbox,
  asFunction: false
)
```
