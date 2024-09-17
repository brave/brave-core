# DevTools Frontend Patching

## Adding a New File

1. Add your file in `//brave/third_party/devtools-frontend/src/front_end/{path to your file}`
2. Create (if it doesn't exist) `sources.gni` with `//brave/third_party/devtools-frontend/src/front_end/{path to your file}`
3. Create a patch in the corresponding `BUILD.gn` in `//third_party/devtools-frontend/src`
4. Add a reference to the compiled file in `//brave/third_party/devtools-frontend/src/config/gni/sources.gni`

### How It Works

When `chromium_src/third_party/devtools-frontend/src/third_party/typescript/ts_library.py` detects a file starting with `//brave`, it copies this file to the corresponding upstream directory. When the compilation of the corresponding target is done, `ts_library.py` removes the copied file to preserve the upstream sources in their original state.

## Patching an Existing File

1. Create a patch file in `//brave/chromium_src/third_party/devtools-frontend/src/front_end/{path to the file you want to patch}.ts`
2. Apply the patch after `SomeClass` declaration in the original file:
   ```javascript
   import { PatchSomeClass } from './SomeFile.patch.js'; (SomeClass as any) = PatchSomeClass(SomeClass);
   ```

### How It Works

When `chromium_src/third_party/devtools-frontend/src/third_party/typescript/ts_library.py` detects a `chromium_src` counterpart, it copies the file to the corresponding upstream directory (replacing `.ts` with `.patch.ts`). You don't need to add this file in `sources.gni` or `BUILD.gn`. Instead, you should import the `{filename}.patch.js` file (See step #2 above). `chromium_src/third_party/devtools-frontend/src/scripts/build/generate_devtools_grd.py` will automatically add the `.patch.js` file into the resource bundle.
