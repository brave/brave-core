# BraveSoftwareMSI

Test MSI installers for Omaha 4's `updater_tests`. Upstream's
`//chrome/updater/test/test_installer` looks them up under
`//chrome/updater/test/data/${updater_company_short_name}MSI/` and only checks
in `ChromiumMSI` and `GoogleMSI`. `build/commands/lib/branding.js` copies this
directory there.

Upstream regenerates the MSIs at build time when WiX is available, but WiX is
only checked out with `checkout_src_internal`. To regenerate ours, install WiX
Toolset v3 and run, from `src/`:

```
python3 chrome/updater/test/test_installer/create_test_msi_installer.py \
  --candle_path "C:/Program Files (x86)/WiX Toolset v3.14/bin/candle.exe" \
  --light_path "C:/Program Files (x86)/WiX Toolset v3.14/bin/light.exe" \
  --product_name "Test System MSI Installer" \
  --product_version <1.0.0.0 or 2.0.0.0> \
  --appid "{c28fcf72-bcf2-45c5-8def-31a74ac02012}" \
  --msi_base_name TestSystemMsiInstaller \
  --msi_template_path chrome/updater/test/test_installer/test_installer.wxs.xml \
  --company_name BraveSoftware \
  --company_full_name "Brave Software, Inc" \
  --checked_in_msi brave/test/data/updater/BraveSoftwareMSI/{c28fcf72-bcf2-45c5-8def-31a74ac02012}.<version>/TestSystemMsiInstaller.msi \
  --output_dir <any scratch directory>
```

The arguments mirror the `test_msi_installer` template in
`//chrome/updater/test/test_installer/BUILD.gn`.
