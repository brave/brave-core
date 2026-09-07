# DevTools Frontend Patching

## Adding a New File

1. Add your file in
   `//brave/third_party/devtools-frontend/src/front_end/{path to your file}`
2. Create (if it doesn't exist) `sources.gni` with
   `//brave/third_party/devtools-frontend/src/front_end/{path to your file}`
3. Create a patch in the corresponding `BUILD.gn` in
   `//third_party/devtools-frontend/src`
4. Add a reference to the compiled file in
   `//brave/third_party/devtools-frontend/src/config/gni/sources.gni`

### How It Works

Every file a `ts_library` target compiles has to sit under that target's
`rootDir`, so a `//brave` source can't be handed to `tsc` where it lives. Our
patch to `scripts/build/typescript/typescript.gni` makes `ts_library` name such
a source as if it sat next to the upstream sources it extends, and
`chromium_src/third_party/devtools-frontend/src/scripts/build/run_with_restat.py`
hardlinks the real file there before `tsc` runs.

## Patching an Existing File

1. Create a patch file in
   `//brave/chromium_src/third_party/devtools-frontend/src/front_end/{path to the file you want to patch}.ts`
2. Apply the patch after `SomeClass` declaration in the original file:
   ```javascript
   import { PatchSomeClass } from './SomeFile.patch.js'; (SomeClass as any) = PatchSomeClass(SomeClass);
   ```

### How It Works

When
`chromium_src/third_party/devtools-frontend/src/scripts/build/run_with_restat.py`
finds a `chromium_src` counterpart for a file `tsc` is about to compile, it
hardlinks that counterpart next to the upstream file, replacing `.ts` with
`.patch.ts`. You don't need to add this file in `sources.gni` or `BUILD.gn`.
Instead, you should import the `{filename}.patch.js` file (See step #2 above).
`chromium_src/third_party/devtools-frontend/src/scripts/build/generate_devtools_grd.py`
will automatically add the `.patch.js` file into the resource bundle.
