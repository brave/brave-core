# Brave Ads

Users earn tokens by viewing privacy-first Brave Ads. Ads presented are based on the user's interests, as inferred from the user's browsing behavior. No personal data or browsing history should ever leave the browser.

For more details, refer to [glossary of terms](GLOSSARY.md).

Brave Ads is a [layered component](https://sites.google.com/a/chromium.org/dev/developers/design-documents/layered-components-design). It has the following structure:

    └── components/
        └── brave_ads/
            ├── core/ ¹
            │   ├── browser/ ²
            │   ├── mojom/ ³
            │   ├── public/ ⁴
            │   ├── internal/ ⁵
            │   └── test/ ⁶
            ├── content/ ⁷
            ├── browser/ ⁸
            │   └── resources/ ⁹
            └── buildflags/ ¹⁰

1. The shared code that does not depend on //content.
2. Browser process code, such as the ads service and the ads internals WebUI handler, that does not depend on //content.
3. The public mojom data structures.
4. The public API surface of the component.
5. The internal implementation of public/. Not visible to consumers.
6. The data used in tests.
7. The code layered above core/ that depends on //content.
8. Browser process code which is specific for Desktop and Android platforms. Code therein will be eventually transitioned to core/browser/ and content/.
9. The ads internals WebUI frontend.
10. Build-time feature flags, such as whether Brave Ads is enabled for this platform.

## Command Line Switches

| switch  | explanation  |
|---|---|
| rewards  | Multiple options can be comma separated (no spaces). Note: all options are in the format `foo=x`. Values are case-insensitive. Options are `staging`, which forces ads to use the staging environment if set to `true` or the production environment if set to `false`. `debug`, which, if set to `true`, reduces the delay before downloading the catalog, fetching subdivisions, processing the confirmation queue, and paying out confirmation tokens, and ignores the new tab page ad grace period. `uuids`, which filters ads to a comma-separated list of UUIDs representing a creative instance, creative set, campaign, or advertiser. e.g. `--rewards=staging=true,debug=true,uuids=fd955b44-5b46-4359-a074-3bc700cb86bf`.  |
| vmodule  | Options are `--vmodule` gives the per-module maximal V-logging levels to override the value given by `--v`, e.g., `"*/brave_ads/*"=6,"*/bat_ads/*"=6` would change the logging level for Brave Ads to 6.  |
| use-dev-goupdater-url  | Options are `--use-dev-goupdater-url`, which forces the component updater to use the dev environment.  |

## Logs and Diagnostics

You can enable diagnostic logging to the `Rewards.log` file stored on your device at `brave://flags/#brave-rewards-verbose-logging`. View this log file and other diagnostics at `brave://ads-internals`, on the `Logs` and `Diagnostics` tabs respectively.

## Browser Tests

    pnpm test brave_browser_tests --filter=BraveAds*

## Unit Tests

    pnpm test brave_all_unit_tests --filter=BraveAds*

Please add to it!
