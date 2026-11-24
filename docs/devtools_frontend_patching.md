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

## Testing

Brave's DevTools changes are unit tested with upstream's Mocha and Karma setup
(see `//third_party/devtools-frontend/src/test/unit/README.md`), run in the
Brave build.

1. Add the test next to the file it tests, as
   `//brave/third_party/devtools-frontend/src/front_end/{path}/Foo.test.ts`. A
   test of a `chromium_src` override is named after the override as compiled:
   `Foo.patch.test.ts` for `Foo.patch.ts`.
2. List it in the `sources.gni` next to it, and add that list to the directory's
   `unittests` target with plaster, as for sources.
3. Run it:
   ```sh
   pnpm run test devtools_frontend_unittests [<test file or dir> ...]
   ```
   Without a test, it runs all of Brave's. Upstream's tests can be named too
   (`front_end/panels/network`), which runs them against Brave's overrides.
   Other options go to `dt test` (`--grep=<text>`, `--debug`). On hosts that
   restrict unprivileged user namespaces (Ubuntu 23.10+), add `--no-sandbox`.

A test reaches a `chromium_src` override through the module's entry point
(`./network.js`), which exports the patched class. Test Brave's additions to a
view against upstream's `DEFAULT_VIEW` rather than a view stub: a stub never
renders the elements Brave adds.
