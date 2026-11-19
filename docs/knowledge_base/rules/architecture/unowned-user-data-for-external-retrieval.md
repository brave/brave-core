---
name: brave/unowned-user-data-for-external-retrieval
description:
  'A tab or window feature that other code looks up exposes a static From()
  through UnownedUserData; a self-contained one is a plain unique_ptr member;
  neither is reached through FromWebContents().'
rubric: design
severity: should_fix
---

**Rule.** A tab or window feature is a `std::unique_ptr` member of
`BraveTabFeatures` or `BraveBrowserWindowFeatures`. When it is self-contained
(it observes what it needs and nothing looks it up), that is all. When other
code needs to retrieve it from a `TabInterface` or `BrowserWindowInterface`, use
the Unowned User Data pattern: `DECLARE_USER_DATA` / `DEFINE_USER_DATA`, a
`ScopedUnownedUserData` member registered on the host's
`GetUnownedUserDataHost()`, and a static `From(tab)`. Do not expose it through
`FromWebContents()`.

**Why.** Unowned User Data separates who owns and creates the feature from how
it is found, so retrieval follows the tab or window rather than a `WebContents`,
and adds no lookup machinery to features nobody looks up.

## Examples

Bad:

```cpp
auto* feature = MyTabFeature::FromWebContents(web_contents);
```

Good:

```cpp
class MyTabFeature {
 public:
  DECLARE_USER_DATA(MyTabFeature)

  static MyTabFeature* From(tabs::TabInterface* tab) {
    return Get(tab->GetUnownedUserDataHost());
  }

 private:
  ScopedUnownedUserData<MyTabFeature> scoped_unowned_user_data_;
};
```
