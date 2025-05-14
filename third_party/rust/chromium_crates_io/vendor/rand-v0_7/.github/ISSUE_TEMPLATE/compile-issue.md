---
name: Compile issue
about: Report / ask about a compilation issue
title: ''
labels: ''
assignees: ''

---

# Common issues

**Problem**: `rand_hc::Hc128Rng: rand_core::SeedableRng` (or other RNG)

**Quick solution**: `cargo update`

**Details**: This happens when multiple versions of the `rand_core` crate are in use. Check your `Cargo.lock` file for all versions of `rand_core`. Note that some versions (0.2.2 and 0.3.1) are compatibility shims and are not a problem by themselves.
