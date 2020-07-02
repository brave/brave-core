Use this directory to add styles to any chromium polymer module.
Steps to take:
- Create a JS module which calls `RegisterStyleOverride`, passing in the module name and the new style element to "inject".
- Import the module to the index.js inside this directory.
- Include the module in settings_resources.grd.