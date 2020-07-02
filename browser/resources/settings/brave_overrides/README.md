Use this directory to add modifications to any chromium polymer module.
Steps to take:
- Create a JS module which calls any of the polymer_overriding exported functions, e.g. `RegisterStyleOverride`, depending on what you want to inject:
   - Definition-time html polymer template changes? Use `RegisterPolymerTemplateModifications`.
   - Runtime functionality hooks? Use `RegisterPolymerComponentBehaviors`.
   - CSS style? Use `RegisterStyleOverride`.
- Import the module to the index.js inside this directory.
- Include the module in settings_resources.grd.