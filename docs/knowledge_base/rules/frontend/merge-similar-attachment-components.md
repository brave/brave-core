---
name: brave/merge-similar-attachment-components
description:
  When an upload or attachment UI gains a new file type, merge the similar
  per-type components into one generic component instead of adding a parallel
  one.
rubric: design
severity: should_fix
---

**Rule.** When a change adds support for a new file type to an upload or
attachment UI, merge the similar components into a single generic one that takes
the type as a prop (`<AttachmentUploadItem type={file.type} />`). Do not add
another parallel component per type next to the existing ones
(`AttachmentDocumentItem` beside `AttachmentImageItem`).

**Why.** Parallel components repeat the same markup and behaviour, so every fix
and design change has to be made in each of them, and they drift apart when one
is missed.

## Examples

Bad:

```tsx
<AttachmentImageItem />
<AttachmentDocumentItem />
```

Good:

```tsx
<AttachmentUploadItem type={file.type} />
```
