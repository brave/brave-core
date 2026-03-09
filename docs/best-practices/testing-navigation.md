# Navigation and Timing

<a id="TN-001"></a>

## Same-Document Navigation

**DO NOT use `base::test::RunUntil()` polling for same-document (hash/fragment) navigations.**

Standard `TestNavigationObserver` skips same-document navigations. Use a custom observer:

```cpp
class SameDocumentCommitObserver : public content::WebContentsObserver {
 public:
  explicit SameDocumentCommitObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents) {}

  void Wait() { run_loop_.Run(); }

 private:
  void DidFinishNavigation(content::NavigationHandle* handle) override {
    if (handle->IsSameDocument() && !handle->IsErrorPage()) {
      run_loop_.Quit();
    }
  }

  base::RunLoop run_loop_;
};

// Usage:
SameDocumentCommitObserver observer(web_contents);
// Trigger navigation...
observer.Wait();
```

---

<a id="TN-002"></a>

## Avoid Hardcoded JavaScript Timeouts

**BAD:**
```cpp
// ❌ WRONG - Unreliable hardcoded timeout
content::EvalJs(web_contents, R"(
  setTimeout(() => { /* wait for something */ }, 1200);
)");
```

**GOOD:**
```cpp
// ✅ CORRECT - Wait for actual condition in C++
ASSERT_TRUE(base::test::RunUntil([&]() {
  return template_url_service->GetTemplateURLForHost(host) != nullptr;
}));
```

---

<a id="TN-003"></a>

## Wait for Page Distillation

When testing distilled content (Speedreader), always wait for distillation to complete:

```cpp
NavigateToPageSynchronously(url);
WaitDistilled();  // Wait for distillation before interacting with content
// Now safe to interact with distilled elements
```

`NavigateToPageSynchronously` only waits for load stop, not for distillation.
