# Patching Chromium

One of the primary goals of brave-core is to do changes in a way that makes it easy for Chromium rebases. This is because speed of updating to a new Chromium version is important to us.

Please follow this order when doing patching from best to worst:

## Changes only inside brave-core

If changes can be made inside existing subclasses and code inside `src/brave`, then that is preferred.

## Changes inside Chromium

### Minimize dependencies

We want to avoid creating new dependencies in the chrome code when possible. gn
dependency checks also don't currently run for chromium_src so unlike other
targets, it won't fail if you add includes that do not have deps listed for the
original source file target. We also want to avoid patching gn to add
dependencies. One way to avoid these is with forward declarations. For most code
in chrome you can forward declare classes or methods that have their
implementation in brave. Code in `component` gn target types doesn't lend itself
to this technique in general. This also applies to patches in general including
plaster.

chromium_src/chrome/browser/chrome_feature/chrome_feature.cc
```cpp
bool BraveDoSomething(...);

#define DoSomething DoSomething_ChromiumImpl

bool ChromeFeature::DoSomething(...) {
  if (BraveDoSomething(...)) {
    return true;
  }

  return DoSomething_ChromiumImpl(...);
}
```

brave/browser/some_feature/my_feature_override.cc
```cpp
bool BraveDoSomething(...) {
  ...
}
```

brave/browser/sources.gni
```gn
deps += [ "//brave/browser/chrome_feature" ]
```

### Introduction to `chromium_src` overrides

When you can't make a change directly in existing `src/brave` code, different approaches can be used to alter an upstream implementation. Many of them are based on `src/brave/chromium_src` overrides. The content of this directory is prioritized over upstream files during compilation. The basic rules are:
* `#include "chrome/browser/profiles/profile.h"` will actually include `src/brave/chromium_src/chrome/browser/profiles/profile.h` if it exists.
* compile `chrome/browser/profiles/profile.cc` will actually compile `src/brave/chromium_src/chrome/browser/profiles/profile.cc` if it exists.

### Subclass and override

To change an upstream logic it's often best to simply subclass a Chromium class, and override the functions needed.

You will need to patch (see the below documentation) for some small trivial things in this case:
- Create instances of your class instead of the Chromium class.
- Possibly add a `friend` member to the base class you're subclassing.
- Possibly add a `virtual` keyword to functions you'd like to subclass - [example](https://github.com/brave/brave-browser/wiki/Patching-Chromium#making-methods-virtual)

Header patches should use preprocessor defines when possible. The define should always be the last thing in `public` so you can change to `protected` or `private` inside the define.
```
  bool ShouldRunUnloadListenerBeforeClosing(content::WebContents* web_contents);
  bool RunUnloadListenerBeforeClosing(content::WebContents* web_contents);

  // Set if the browser is currently participating in a tab dragging process.
  // This information is used to decide if fast resize will be used during
  // dragging.
  void SetIsInTabDragging(bool is_in_tab_dragging);

  BRAVE_BROWSER_H
 private:
```
with `src/brave/chromium_src/chrome/browser/ui/browser.h` override:
```
#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_H_

#define BRAVE_BROWSER_H \
 private:               \
  friend class BookmarkPrefsService;

#include <chrome/browser/ui/browser.h>

#undef BRAVE_BROWSER_H

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_H_
```
### Using the preprocessor to use base implementations inside override files

One strategy that's preferred over patching is to use `src/brave/chromium_src` which overrides `.cc` and `.h` files but still use the source in the original Chromium code too. To do that you can rename a function with the preprocessor in Chromium, and then provide your own real implementation of that file and use the Chromium implementation inside of it.

Here's an example:
https://github.com/brave/brave-core/blob/5293f0cab08816819bb307d02e404c2061e4368d/chromium_src/chrome/browser/browser_about_handler.cc

No BUILD.gn changes are needed for this.

### Making methods virtual
There are two methods depending on whether you have a pointer or non-pointer return value.
For pointer return values:
```
#define GetExtensionAction           \
  UnusedMethod() { return nullptr; } \
  virtual ExtensionAction* GetExtensionAction
```

and for non-pointer types:
```
#define ReportResult virtual ReportResult
```
https://github.com/brave/brave-core/commit/a85a399a16df59b99b18382e2a4106d63e1a32c1#diff-925a04f8f2bcee20b47c338c1a3c70b9

### Override a `.cc` file completely

If you want to provide a completely different implementation of a file, it is often not safe, but sometimes applicable. You can just provide the alternate implementation inside the `src/brave/chromium_src` directory.

One way electron went wrong is they copied entire files for changes inside a similar setup, do NOT do this. This will lead to newer Chromium rebases over time using old stale code which causes problems and makes rebasing much harder.

No BUILD.gn changes are needed for this.

### Patch the Chromium files

When other options are exhausted, you can patch the code directly in `src/`. After making the changes, you can run the npm command `npm run update_patches`.   This will update the patches which are stored in  `src/brave/patches`.   Please note that removed changes in `src` currently will not update the patches, so you will have to do that manually.

We aim to make the only patches required to be trivial changes, and not nested logic changes.
If possible write the patch to add a new line vs appending/prepending to an existing line.

For example, instead of
```
-  return !url.is_empty() && !url.SchemeIs(content::kChromeUIScheme) &&
+  return IsBraveTranslateEnabled() && !url.is_empty() && !url.SchemeIs(content::kChromeUIScheme) &&
!url.SchemeIs(content::kChromeDevToolsScheme) &&
```
it should be
```
return !url.is_empty() && !url.SchemeIs(content::kChromeUIScheme) &&
+  IsBraveTranslateEnabled() &&
!url.SchemeIs(content::kChromeDevToolsScheme) &&
```
Do not add comments in patches and ignore lint line length rules to squash patches onto one line whenever possible

You should almost never patch in two methods calls in a row. We should prefer extensible patches. For instance https://github.com/brave/brave-core/pull/2693/files#diff-a9c9a8da7aa4df821394352a0ca04a27R12:
```
CopyBraveExtensionLocalization(config, staging_dir, g_archive_inputs)
CopyBraveRewardsExtensionLocalization(config, staging_dir, g_archive_inputs)
```
inside `CopyAllFilesToStagingDir` would be collapsed to
```
CopyBraveFilesToStagingDir
```

Make sure you do NOT have the following in your `~/.gitconfig`:

    [apply]
            whitespace = fix

as trailing whitespace can be essential in patch files.

## Patching `gn/gni` files
We should also prefer extensible patches in gn files where possible.

Multiple deps should never be added to the same target. Always create a generic brave dep and then add other deps (public_deps if needed) inside that.

The same thing goes for sources, but those should be added as `sources += my_brave_sources` where `my_brave_sources` is defined in a brave gni file. We have a gni file that is already included in nearly every gn build file in chromium through a patch in chrome_build.gni (`import("//brave/build/config/brave_build.gni"`). Add new gni imports inside brave_build.gni instead of patching them into another gn/gni file

## Patching `mojom` files
Mojom files can be patched using an override placed at the same location in `src/brave/chromium_src` directory. It's possible to:
* add: `const`, `enum`, `interface`, `struct`, `union`. Use `[BraveAdd]` attribute to ensure we don't overwrite upstream definition.
* extend: `enum`, `interface`, `struct`, `union`. Use `[BraveExtend]` attribute to ensure a required definition exists with the exact type.

Examples:
```
[BraveExtend]
enum GlobalEnum {
  NEW_VALUE = 1,
  NEW_VALUE_GLOBAL_CONSTANT = kGlobalConstant2,
};

[BraveAdd]
union NewGlobalUnion {
  int32 int32_value;
  float float_value;
};
```
* [All-in-one mojom patch example](https://github.com/brave/brave-core/blob/569ccb3766a1e7752b7a4166bd3f07aad2afe560/chromium_src/brave/mojo/brave_ast_patcher/test_module.mojom)
* Extending a mojo struct that uses traits: [mojom](https://github.com/brave/brave-core/blob/569ccb3766a1e7752b7a4166bd3f07aad2afe560/chromium_src/components/content_settings/core/common/content_settings.mojom), [native header](https://github.com/brave/brave-core/blob/569ccb3766a1e7752b7a4166bd3f07aad2afe560/chromium_src/components/content_settings/core/common/content_settings.h), [native source](https://github.com/brave/brave-core/blob/569ccb3766a1e7752b7a4166bd3f07aad2afe560/chromium_src/components/content_settings/core/common/content_settings.cc), [traits header](https://github.com/brave/brave-core/blob/569ccb3766a1e7752b7a4166bd3f07aad2afe560/chromium_src/components/content_settings/core/common/content_settings_mojom_traits.h), [traits source](https://github.com/brave/brave-core/blob/569ccb3766a1e7752b7a4166bd3f07aad2afe560/chromium_src/components/content_settings/core/common/content_settings_mojom_traits.cc)

## Patching Android `java` files

Many java patches can be replaced by using asm. In order to use asm properly you have to ensure that things compile correctly pre-asm and then make the changes in the asm step that will produce the actual calls you want. Examples:
Changing private methods to public - https://github.com/brave/brave-core/pull/4716/files and https://github.com/brave/brave-core/pull/5127/files

Try to extend Java class like in that example https://github.com/brave/brave-core/blob/master/android/java/org/chromium/chrome/browser/BraveActivity.java or https://github.com/brave/brave-core/blob/master/android/java/org/chromium/chrome/browser/toolbar/top/BraveToolbarLayout.java.
After that just create the new class via `new ...` where the old class created.

## Patching Android `xml` files

- AndroidManifest.xml: Brave's addition to the manifest is included in the original chromium's and located in that place https://github.com/brave/brave-core/blob/master/android/java/AndroidManifest.xml. Add new items inside it.
- layouts: Layouts could be included in original chromium's layouts `<include layout="@layout/brave_toolbar" android:layout_height="wrap_content" android:layout_width="match_parent" />`
- styles: Add new style to https://github.com/brave/brave-core/blob/master/android/java/res/values/brave_styles.xml
- preferences: create your preference in a separate xml file https://github.com/brave/brave-core/blob/master/android/java/res/xml/brave_main_preferences.xml and inject it in runtime from java code https://github.com/brave/brave-core/blob/master/android/java/org/chromium/chrome/browser/preferences/BraveMainPreferencesBase.java#L51
- resources: (DEPRECATED - we should be creating our own resource directory, not copying into upstream. Also this does not apply to changes to upstream files unless we are replacing them and do not care about future changes. Do not use this to patch upstream xml files) add general resources, pictures and etc inside https://github.com/brave/brave-core/tree/master/android/java/res. They are copied inside original chromium's folder before a build

## Patching `py` files

Python files should use [`brave_chromium_utils`](https://github.com/brave/brave-core/blob/master/script/brave_chromium_utils.py) calls to inject content from files located in `brave/chromium_src`. Inlined files may modify original logic by overriding functions and variables with helpers from [`override_utils`](https://github.com/brave/brave-core/blob/master/script/override_utils.py).

If a patched file has `if __name__ == '__main__'` line, then you should inline `brave/chromium_src/...` file right before it:
```
from brave_chromium_utils import inline_chromium_src_override; inline_chromium_src_override(globals(), locals())
if __name__ == '__main__':
  sys.exit(main())
```
otherwise the inline call should be the last line in the file.

You can fully replace functions/variables/classes in the inlined file with a help of `override_utils` helpers. These helpers will fail if an object to override is not found which allow us to detect failures very early in the build.
```
# Example of a global function override.
@override_utils.override_function(globals())
def DirectoryIsPartOfPublicGitRepository(orig_func, local_dir):
    if IsGitIgnored(local_dir, '.'):
        return False

    return orig_func(local_dir)

# Example of a class function override.
@override_utils.override_method(PossibleDesktopBrowser)
def _TearDownEnvironment(self, original_method):
  if '--update-source-profile' in self._browser_options.extra_browser_args:
      # Override the source profile by the result profile.
      shutil.rmtree(self._browser_options.profile_dir)
      shutil.copytree(self._profile_directory,
                      self._browser_options.profile_dir)
  original_method(self)
```



To look for other examples just search for helper names from `override_utils` across codebase.

## Patching `ts` files

Until recently (the end of 2024) we didn't have an easy way to patch Typescript/Javascript files which resulted in the `brave_overrides` pattern used by settings.

Since then we've added support for `chromium_src` overrides like we have in other languages. Add a file to `chromium_src` at the same path it is upstream. You can import upstream's files to modify or replace them with `import './<filename>-chromium.js'`.

To reexport everything from the upstream file you can write:

```ts
import { getName as getNameChromium, getGreeting } from './foo-chromium.js'

// Everything from upstream will be exported
export * from './foo-chromium.js'


// replaces the chromium version
export function getName() {
  return `Brave`
}

// Export markdown string crossing out the old name
export function getGreeting() {
  return `Hi ~~${getNameChromium()}~~ ${getName()}`
}
```

When overriding Typescript you need to know whether the upstream file is using Polymer or Lit (they're very gradually migrating to Lit) as we have different utilities for overriding each.

`//resources/brave/lit_overriding.js` has our Lit overriding utilities
`//resources/brave/polymer_overriding.js` has our Polymer overriding utilities

### Accessing private methods

In general we prefer to avoid patching, but Chromium makes a lot of class methods private. You can normally access these methods via an `as any` cast, because the `private` keyword only exists at compile time. If they use JavaScript native private methods you will need a more custom approach.

```ts
const classWeNeedInternalsFrom = new UpstreamClass();
(classWeNeedInternalsFrom as any).callPrivateMethod();
```
