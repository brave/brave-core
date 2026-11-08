---
name: brave/observe-same-document-navigations
description:
  A test waiting for a same-document (hash or fragment) navigation uses a
  WebContentsObserver that quits on its DidFinishNavigation(), not RunUntil()
  polling or TestNavigationObserver.
rubric: testing
severity: should_fix
---

**Rule.** To wait for a same-document navigation (a hash or fragment change), do
NOT poll with `base::test::RunUntil()`, and do not rely on
`content::TestNavigationObserver`, which skips same-document navigations. Use a
`content::WebContentsObserver` whose `DidFinishNavigation()` quits a
`base::RunLoop` when `handle->IsSameDocument()` (and not `IsErrorPage()`),
create it before triggering the navigation, and wait on it.

**Why.** The observer is notified exactly when the same-document navigation
finishes, whereas polling only approximates that moment, and
`TestNavigationObserver` does not track same-document navigations at all.

## Examples

Bad:

```cpp
ASSERT_TRUE(content::ExecJs(web_contents, "location.hash = 'section'"));
ASSERT_TRUE(base::test::RunUntil([&]() {
  return web_contents->GetLastCommittedURL().ref() == "section";
}));
```

Good:

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

SameDocumentCommitObserver observer(web_contents);
ASSERT_TRUE(content::ExecJs(web_contents, "location.hash = 'section'"));
observer.Wait();
```
