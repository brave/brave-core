---
name: brave/test-off-the-record-clipboard-marking
description:
  Code marking clipboard writes as off-the-record has unit tests of both
  branches, using a ui::TestClipboard subclass that records the privacy_types it
  is passed.
rubric: testing
severity: should_fix
---

**Rule.** When a change makes a clipboard write call
`ScopedClipboardWriter::MarkAsOffTheRecord()` for off-the-record profiles, its
unit tests cover both branches: with a `ui::TestClipboard` subclass that records
the `privacy_types` passed to `WritePortableAndPlatformRepresentations()`,
assert that an off-the-record profile's write carries the off-the-record types,
and that a regular profile's write does not (the negative case).

**Why.** Nothing visible shows whether a write was marked, so only a test
checking the privacy types keeps the marking from regressing, and the
regular-profile case keeps it from spreading to regular windows.

## Examples

Bad:

```cpp
// Only checks that the image reached the clipboard, for one profile type.
TEST_F(ScreenshotControllerTest, CopyToClipboard) { ... }
```

Good:

```cpp
// brave::PrivacyCapturingTestClipboard is a ui::TestClipboard subclass that
// records the privacy_types of each write.
TEST_F(ScreenshotControllerTest, CopyToClipboard_OffTheRecordProfile) {
  brave::ScopedPrivacyCapturingTestClipboard fake_clipboard;
  // ... copy a screenshot taken in an off-the-record profile ...
  EXPECT_EQ(fake_clipboard.last_privacy_types(),
            static_cast<uint32_t>(ui::Clipboard::kNoCloudClipboard |
                                  ui::Clipboard::kNoLocalClipboardHistory));
}

TEST_F(ScreenshotControllerTest, CopyToClipboard_RegularProfile) {
  brave::ScopedPrivacyCapturingTestClipboard fake_clipboard;
  // ... copy a screenshot taken in a regular profile ...
  EXPECT_EQ(fake_clipboard.last_privacy_types(),
            static_cast<uint32_t>(ui::Clipboard::kNone));
}
```
