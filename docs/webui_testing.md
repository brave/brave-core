## Testing in WebUI

Upstream provides a system for creating frontend tests using Mocha. In Brave, we
tend to use Jest for writing frontend tests. We introduced Jest before upstream
had a good story for writing WebUI tests.

However, due to the different approaches taken both tests fit into a different
niche. Upstream's approach is good for writing full end-to-end tests and can be
thought of as similar to a `browser_test` (in fact, they use a lot of the
browser_test machinery). Our Jest tests are conceptually much more similar to
`unit_tests` and we make extensive use of mocking to test small pieces of
functionality.

## Jest `unit_tests`

These were introduced in Brave before there was a good story for testing WebUIs
upstream. They are primarily used with our React pages, but there's no reason
they can't be used to unit test Lit/Polymer functionality.

Adding one of these tests is simple: Create a `<your-file>.test.ts` file next to
the file you want to test.

```ts
// <your-file>.test.ts
import { add } from './your-file'

describe('your file should work the way I want', () => {
  it('should be able to add things', () => {
    expect(add(1, 2)).toBe(3)
  })
})
```

You can run all tests with `npm run test-unit` or filter to a specific test with
`npm run test-unit -- -t "should be able to add"`. The tests run quite quickly
and you can run them in `watch` mode to automatically run tests affected by a
change with `npm run test-unit -- --watch`.

We use [Testing Library](https://testing-library.com/) for writing React unit
tests. It can render React trees to a Jest DOM, allowing you to take snapshots
and to check that things are rendering as expected.

### Additional Resources

- https://jestjs.io/
- https://testing-library.com/

## Mocha `browser_tests`

These run in a real browser via the `browser_tests` machinery, so they're the
right choice for testing **upstream WebUI pages we modify** (e.g. the desktop
settings page). Those pages depend heavily on the browser environment (Polymer,
`chrome://` resources, `cr.js`, browser proxies), which our Jest/JSDOM tests
can't reproduce. Conceptually they're still unit-test-shaped — you instantiate
an element and assert on it — but they execute in-browser.

For most of our React pages, prefer the Jest `unit_tests` above. Reach for Mocha
when the thing under test is an upstream Lit/Polymer element or only works
inside a live WebUI.

See Upstream's documentation for how to write the test bodies:
https://chromium.googlesource.com/chromium/src/+/main/docs/webui/testing_webui.md

### Writing a settings (or other upstream) WebUI mocha test

Brave-owned settings WebUI tests live in
[`brave/test/data/webui/settings/`](../test/data/webui/settings/) and are
compiled into `brave_browser_tests` so they run on Brave CI. Use the existing
`brave_sync_controls_test.ts` / `brave_settings_browsertest.cc` pair as a
template. To add a test for a new piece of upstream-page behavior:

1. **Write the mocha test** as `<name>_test.ts`. Import from the page under test
   (`chrome://settings/...`) and from upstream test helpers via
   `chrome://webui-test/settings/...` (e.g. `getSyncAllPrefs`,
   `TestSyncBrowserProxy`). Create the element, drive it (set props, fire
   `webUIListenerCallback(...)`, `flush()`, `await waitBeforeNextRender(...)`),
   then assert with `chrome://webui-test/chai_assert.js`.

2. **Write the C++ fixture** as `<name>_browsertest.cc`. Subclass
   `WebUIMochaBrowserTest`, call
   `set_test_loader_host(chrome::kChromeUISettingsHost)`, enable any required
   features with `ScopedFeatureList` in the constructor, and in each
   `IN_PROC_BROWSER_TEST_F` call
   `RunTest("brave_settings/<name>_test.js", "mocha.run()")`. Gate on the
   relevant `BUILDFLAG` if the feature is build-conditional.

3. **Wire up `BUILD.gn`** in that directory with two targets:

   - `build_webui_tests("brave_settings")` — lists the `.ts` `files` and
     `cc_test_files`, sets `resource_path_prefix = "brave_settings"`, and
     declares `ts_path_mappings` / `ts_deps` so imports resolve (mirror the
     upstream settings mappings, plus `chrome://webui-test/settings/*` to reuse
     its helpers).
   - `source_set("browser_tests")` — compiles the `.cc` into
     `brave_browser_tests`; must set
     `defines = [ "HAS_OUT_OF_PROC_TEST_RUNNER" ]`.

   Add a `DEPS` file for any `+brave/...` C++ includes.

The plumbing below is already done — you only need to touch it when standing up
a **new** test directory:

- **`test/BUILD.gn`**: `brave_browser_tests` depends on the `:browser_tests`
  source_set and on `//chrome:browser_tests_pak` (the pak provides the
  `chrome://webui-test` resources `WebUIMochaBrowserTest` loads).
- **Plaster `//chrome/test/data/webui`** (`rewrite/.../BUILD.gn.yaml`) bundles
  our `build_grdp` into the shared `chrome://webui-test` resources.
- **Plaster `//chrome/test/data/webui/settings`** makes the upstream settings
  test target `ts_composite = true` so we can reference its `build_ts` and reuse
  its helpers.
- **`third_party/polymer/v3_0/sources.gni`** allows our `:build_ts` in
  `brave_polymer_library_visibility`.
- **`eslint.config.mjs`** ignores `test/data/webui/**/*.ts` (it's linted by the
  GN build's `eslint_ts` instead).

### Running

```bash
npm run test -- brave_browser_tests --filter="BraveSettingsTest.SyncControls"
```
