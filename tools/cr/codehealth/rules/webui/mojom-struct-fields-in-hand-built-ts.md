---
name: webui/mojom-struct-fields-in-hand-built-ts
rubric: webui
severity: regression
---

**Rule.** Brave TS that builds a mojom struct by hand (storybook mocks, test
proxies) sets every field upstream adds to it.

**Why.** The generated `*.mojom-webui.ts` type makes a nullable field required
(`(T | null)`), so a hand-built literal without it fails type-checking. Stories
aren't in the GN build, so only `build-storybook` fails, after the lift.

**How to check.** For each struct the candidates add a field to, grep Brave's
`*.ts`/`*.tsx` for literals of it (an existing field name followed by `:`, e.g.
`fuseboxAction:`), and check each sets the new field. Upstream's own hand-built
literals (`*_browser_proxy.ts`, `*_test_utils.ts`) show the value to use.

## Examples

- M160, 8163.1: `SuggestTemplateInfo.action_icon` in
  `brave_new_tab_page_refresh/stories/mock_search_store.ts`
  ([69cb2ba3a182](https://chromium.googlesource.com/chromium/src/+/69cb2ba3a1825b39de009b3c806f86d69d99437f)).
- M160, 8164.1: `SuggestTemplateInfo.type_icon_badge`, the same mock
  ([97d9c4c4fd72](https://chromium.googlesource.com/chromium/src/+/97d9c4c4fd72567860d32ae9648abb357005fec9)).
