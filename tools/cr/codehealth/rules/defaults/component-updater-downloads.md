---
name: defaults/component-updater-downloads
rubric: defaults
severity: regression
---

**Rule.** Fetches Brave stubs out don't come back through a new component
installer.

**Why.** A flag flip can route a fetch Brave stubs out (e.g.
`PredictionManager`) through a new component instead, which only
`kDisallowedComponents` blocks.

**How to check.** For each new or newly enabled installer in the candidates,
compute its CRX id from the public key hash and check it against
`kDisallowedComponents` in
`chromium_src/components/component_updater/component_installer.cc`.

## Examples

- M158, 8111.0: `kPredictionModelComponentDelivery`.
