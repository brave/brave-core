---
name: brave/document-exported-typescript-types
description:
  Exported TypeScript types meant for use outside their component must have
  documentation comments explaining their purpose.
rubric: readability
severity: should_fix
---

**Rule.** Give every exported `type`, `interface` or `enum` that code outside
the component is meant to use a documentation comment (`/** ... */`) saying what
it is for. Complex union types, and data shapes that form a component's API,
especially need one, explaining each case or field a consumer has to fill in.

**Why.** Consumers see an exported type from their own code, away from the
component that gives it meaning; without its documentation they have to read the
implementation to know what to pass.

## Examples

Bad:

```tsx
export type AttachmentState =
  | { kind: 'uploading'; progress: number }
  | { kind: 'ready'; file: File }
  | { kind: 'error'; message: string }
```

Good:

```tsx
/**
 * The state of an attachment in the input box, as the attachment list
 * renders it.
 */
export type AttachmentState =
  /** Upload in progress; `progress` is between 0 and 1. */
  | { kind: 'uploading'; progress: number }
  /** Uploaded and ready to send with the next message. */
  | { kind: 'ready'; file: File }
  /** The upload failed; `message` is shown to the user. */
  | { kind: 'error'; message: string }
```
