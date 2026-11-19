---
name: brave/mojo-for-trusted-untrusted-webui
description:
  'Trusted and untrusted WebUI frames communicate through a Mojo interface, not
  postMessage, unless the frame deliberately runs untrusted code with a minimal
  API surface.'
rubric: security
severity: should_fix
---

**Rule.** For communication between a trusted (`chrome://`) WebUI frame and an
untrusted (`chrome-untrusted://`) one, define a Mojo interface. Do not exchange
messages with `postMessage` and `message` event listeners, unless the untrusted
frame intentionally executes untrusted code and keeping its API surface minimal
is a deliberate, stated security choice.

**Why.** Chromium advises against `postMessage` across trust boundaries: its
messages are untyped and any frame can send them, while a Mojo interface is
typed, validated and bound to a specific frame.

## Examples

Bad:

```ts
untrustedFrame.contentWindow.postMessage({ type: 'summarize', text }, '*')
```

Good:

```ts
const response = await untrustedHandler.summarize(text)
```
