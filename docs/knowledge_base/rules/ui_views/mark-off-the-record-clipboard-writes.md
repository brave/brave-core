---
name: brave/mark-off-the-record-clipboard-writes
description:
  Browser UI code writing content from an off-the-record (Private, Tor) profile
  to the clipboard calls ScopedClipboardWriter::MarkAsOffTheRecord(), keeping it
  out of clipboard history and cloud sync.
rubric: security
severity: blocking
---

**Rule.** When browser UI code writes to the clipboard content derived from an
off-the-record profile (Incognito, Private, Tor), it MUST call
`writer.MarkAsOffTheRecord()` on its `ui::ScopedClipboardWriter` when
`profile->IsOffTheRecord()`. Tor profiles are off-the-record, so that check
covers them without a separate one.

**Why.** `MarkAsOffTheRecord()` sets
`Clipboard::kNoLocalClipboardHistory | Clipboard::kNoCloudClipboard`, so that OS
clipboard history (such as Windows Win+V) and cloud clipboard sync to other
devices do not keep the data. Without it, private-window content such as a
screenshot leaks outside the private session.

## Examples

Bad:

```cpp
ui::ScopedClipboardWriter writer(ui::ClipboardBuffer::kCopyPaste);
writer.WriteImage(bitmap);
```

Good:

```cpp
ui::ScopedClipboardWriter writer(ui::ClipboardBuffer::kCopyPaste);
writer.WriteImage(bitmap);
if (profile->IsOffTheRecord()) {
  writer.MarkAsOffTheRecord();
}
```
