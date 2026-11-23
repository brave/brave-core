# Further optimization

Remaining directions after the native optimization study; gains below are not
established. Preserve the 1.x contracts described in [MIGRATION.md](MIGRATION.md).

- **Error positions:** carrying a failing-block offset avoids rescanning, but
  changed return representations slowed valid short inputs. Coarse cold scans
  also regressed shorter errors. Seek a representation that retains precise
  diagnostics without adding work to successful Hash256 calls.
- **Native coverage:** reproduce marginal differences on dedicated Intel/AMD
  CPUs, especially the i9-14900K. Keep default and ThinLTO builds, multiple call
  sites and code size in the comparison.

Make API tradeoffs explicit. Use identical benchmark harnesses,
independent builds and repeated native x86/ARM comparisons. Retain a change only
when its benefit is reproducible and its complexity earns long-term maintenance.
A short-input gain can qualify without a Hash256 gain if hot/rotating Hash256 and
other important lengths show no reproducible regression. Prefer reusing safe code
and existing SIMD kernels over adding new unsafe paths.
