---
name: brave/service-code-in-services-directory
description:
  'Mojo service implementations and data decoders live in a services/ directory,
  not in components/.../browser/.'
rubric: design
severity: should_fix
---

**Rule.** Put the implementation of a Mojo service (the code that runs in the
utility process behind a service interface) and data decoders in a `services/`
directory, such as `components/services/<name>/`. Do not add them to a
component's `browser/` directory.

**Why.** `browser/` is browser-process code; a service implementation there sits
at the wrong layer and tends to pick up browser-only dependencies the utility
process cannot have.

## Examples

Bad:

```
components/brave_shields/core/browser/adblock_decoder_service_impl.cc
```

Good:

```
components/services/brave_shields/adblock_decoder_service_impl.cc
```
