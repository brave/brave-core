---
name: security/fingerprinting-read-back
rubric: security
severity: regression
---

**Rule.** A new way to read back data Brave farbles goes through Brave's
farbling hook.

**Why.** A read-back path that skips the hook leaks the unfarbled value, which
defeats the protection for that surface.

**How to check.** Launches show up as `status:` flips in
`runtime_enabled_features.json5`, which a `name:` diff misses: map each newly
stable IDL surface to `git grep -l PerturbPixels chromium_src`. For new
read-back methods in Blink, check whether the data comes from a source Brave
farbles (canvas, audio, WebGL) and whether the path calls Brave's hook.

## Examples

- `VideoFrame.copyTo()` on a canvas source skips canvas farbling.
- M157, 8097.1: HTML-in-canvas read back through WebGPU.
