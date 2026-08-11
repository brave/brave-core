# BNES C++ security module (`//brave/bnes`)

Additive BnesBrowser helpers for host / CID / IPFS gateway boundary checks.

| Target | Role |
|--------|------|
| `//brave/bnes:security` | `bns_security` + `bns_constants` |
| `//brave/bnes:unit_tests` | gtest cases (wired into `brave_components_unittests`) |

## Status

- **H5.1**: sources present in this directory.
- **H5.2**: requires full Chromium + GN environment — see [../docs/H5.2_BUILD_ENVIRONMENT.md](../docs/H5.2_BUILD_ENVIRONMENT.md).
- **H5.3**: compile + run unit tests (blocked on H5.2).

## Preflight

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\check-h5.2-env.ps1
```

Do not mark C++ tests **已驗證** until gtest runs under real Chromium GN.
