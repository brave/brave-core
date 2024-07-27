# Brave Ads

Users earn tokens by viewing privacy-first Brave Ads. Ads presented are based on the user's interests, as inferred from the user's browsing behavior. No personal data or browsing history should ever leave the browser.

For more details, refer to [glossary of terms](GLOSSARY.md).

Brave Ads is a [layered component](https://sites.google.com/a/chromium.org/dev/developers/design-documents/layered-components-design). It has the following structure:

    └── components/
        └── brave_ads/
            ├── core/ ¹
            │   ├── mojom/ ²
            │   ├── public/ ³
            │   ├── internal/ ⁴
            │   └── test/ ⁵
            ├── content/ ⁶
            ├── browser/ ⁷
            └── resources/ ⁸

1. The shared code that does not depend on the Content API.
2. The public mojom data structures.
3. The public API surface of the component.
4. The internal implementation of public/. Not visible to consumers.
5. The data used in tests.
6. The code layered above core/ that integrates with the Content API.
7. Browser process code which is specific for Desktop and Android platforms. Code therein will be eventually transitioned to core/ and content/.
8. The runtime resource data.

## Command Line Switches

| switch  | explanation  |
|---|---|
| rewards  | Multiple options can be comma separated (no spaces). Note: all options are in the format `foo=x`. Values are case-insensitive. Options are `staging`, which forces ads to use the staging environment if set to `true` or the production environment if set to `false`. `debug`, which reduces the delay before downloading the catalog, fetching subdivisions, paying out confirmation tokens, and submitting conversions if set to `true`. e.g. `--rewards=staging=true,debug=true`.  |
| vmodule  | Options are `--vmodule` gives the per-module maximal V-logging levels to override the value given by `--v`, e.g., `"*/brave_ads/*"=6,"*/bat_ads/*"=6` would change the logging level for Brave Ads to 6.  |
| use-dev-goupdater-url  | Options are `--use-dev-goupdater-url`, which forces the component updater to use the dev environment.  |

## Logs

You can enable diagnostic logging to the `Rewards.log` file stored on your device; see [Brave Rewards](brave://flags/#brave-rewards-verbose-logging). View this log file on the `Logs` tab at [rewards internals](brave://rewards-internals).

## Diagnostics

View diagnostics at [rewards internals](brave://rewards-internals) on the `Ad diagnostics` tab.

## Browser Tests

    npm run test -- brave_browser_tests --filter=BraveAds*

## Unit Tests

    npm run test -- brave_unit_tests --filter=BraveAds*

Please add to it!
