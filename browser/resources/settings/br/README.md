Use this directory to add modifications to any chromium polymer module.
Steps to take:
- Create a JS module which calls any of the polymer_overriding exported functions, e.g. `RegisterStyleOverride`, depending on what you want to inject:
   - Definition-time html polymer template changes? Use `RegisterPolymerTemplateModifications`.
   - Runtime functionality hooks? Use `RegisterPolymerComponentBehaviors` for legacy polymer-style additions OR subclass the chromium custom element and pass it to `RegisterPolymerComponentReplacement` (and separately make sure the chromium component doesn't get registered, via `RegisterPolymerComponentToIgnore`).
   - Runtime properties `RegisterPolymerComponentProperties`. This will only work for additional properties.
   - CSS style? Use `RegisterStyleOverride`.
- Import the module to the index.js inside this directory.
- Include the module in settings_resources.grd.