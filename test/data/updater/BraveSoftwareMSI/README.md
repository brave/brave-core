# BraveSoftwareMSI

Test MSI installers for upstream's `IntegrationTestMsi.*` suite in
`updater_tests_system`. They are not related to anything Brave ships; the suite
exercises Omaha 4 installing an application that is packaged as an MSI.

`//chrome/updater/test/test_installer:test_msi_installer<version>` looks the
files up under `//chrome/updater/test/data/${updater_company_short_name}MSI/`,
and upstream only checks in `ChromiumMSI` and `GoogleMSI`. Without our copy,
`updater_tests` and `updater_tests_system` do not build.
`build/commands/lib/branding.js` copies this directory to the upstream location.

## Regenerating

The GN target regenerates the MSIs itself when WiX is present and the checked-in
`.wxs` differs from upstream's `test_installer.wxs.xml`, which is the case after
a Chromium upgrade that changes the template. WiX is normally only checked out
with `checkout_src_internal`, so:

1. Put WiX Toolset v3's `candle.exe` and `light.exe` at `src/third_party/wix/`.
2. To force regeneration with an unchanged template, delete the two
   `TestSystemMsiInstaller.wxs` files here and under
   `src/chrome/updater/test/data/BraveSoftwareMSI/`.
3. Build `chrome/updater/test/test_installer:test_msi_installer1.0.0.0` and
   `test_msi_installer2.0.0.0` in a Static configuration.
4. Copy the regenerated `src/chrome/updater/test/data/BraveSoftwareMSI/` back
   over this directory.
