# 🩹 Plaster and semantical patching

Plaster is an experimental tool being introduced in Brave to allow us to apply
changes to upstream sources files, by relying on regex transformations for to
search for patterns and apply substitutions.

## Why 🩹 Plaster

The two traditional approaches to introduce changes to Chromium have been `git`
patches (i.e. `patches/`), and language overrides (i.e. `chromium_src/`). The
use of patches is in general avoided whenever possible, as they tend to easily
run into conflicts when rebasing Brave onto a newer Chromium version. With
language overrides, although more flexible than patches, they tend to be
invisible in the source code target, hard to interpret, and lead to many cases
of unintended replacements.

Using regexes, Plaster avoids the brittleness of patch files, while providing
matching mechanisms that are more flexible than the methods currently employed
for macro replacement. This also means a language-agnostic way to approach
source changes semantically, and in place. Plaster changes can be both seen in
place, as well as audit as patch files.

## How does it work

Plaster files are placed under `rewrite/`, using a `.toml` extension, and they
are supposed to match the path for the file being plastered.

> [!WARNING]
> At the moment, only plaster files to sources in Chromium's `src` repo are
> supported.

Each plaster file will be used to apply changes into a given source, and then
generate a patch for the effected changes.

For example, imagine you want to add changes to
`chrome/browser/autocomplete/autocomplete_classifier_factory.cc`. You would
care about the following files.

 * **Plaster file:** `brave/rewrite/chrome/browser/autocomplete/autocomplete_classifier_factory.cc.toml`
 * **Source file:** `chrome/browser/autocomplete/autocomplete_classifier_factory.cc`
 * **Patch file** `brave/patches/chrome-browser-autocomplete-autocomplete_classifier_factory.cc.patch`

### Creating a plaster file

A plaster file is a `toml` file used to list regexes operations to be applied
in the corresponding source, based on its path. The following is plaster file
to apply a few changes to
`chrome/browser/autocomplete/autocomplete_classifier_factory.cc`.

Creating the source file with `vscode`:
```sh
code rewrite/chrome/browser/autocomplete/autocomplete_classifier_factory.cc.toml
```

This file below has two substitutions to be applied on its source: The first
adds a header to the list of headers in the source. The second one replaces all
occurrences of `ChromeAutocompleteSchemeClassifier` with
`BraveAutocompleteSchemeClassifier`.

```toml
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

[[substitutions]]
description = 'Adding header for BraveAutocompleteSchemeClassifier'
pattern = '#include "extensions/buildflags/buildflags.h"'
re_pattern = '''#include "extensions/buildflags/buildflags.h"
#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"'''

[[substitutions]]
description = 'Patching in BraveAutocompleteSchemeClassifier'
re_pattern = 'ChromeAutocompleteSchemeClassifier'
replace = 'BraveAutocompleteSchemeClassifier'
```

The basic format for a `[[substitution]]` entry is as follow:

```toml
[[plaster]]
description = ''
re_pattern = ''
replace = ''
re_flags = ''  # These are traditional python regex flags.
count = 0
```

To apply this plaster file, just run `plaster.py`:

```sh
tools/cr/plaster.py apply
```

Running `plaster.py` will cause the substitutions to be applied in
`chrome/browser/autocomplete/autocomplete_classifier_factory.cc`, but it will
cause the creation/update of the corresponding patch file for this source. For
exmample, it may produce a diff file like this:

```
diff --git a/patches/chrome-browser-autocomplete-autocomplete_classifier_factory.cc.patch b/patches/chrome-browser-autocomplete-autocomplete_classifier_factory.cc.patch
new file mode 100644
index 00000000000..63703e6940b
--- /dev/null
+++ b/patches/chrome-browser-autocomplete-autocomplete_classifier_factory.cc.patch
@@ -0,0 +1,21 @@
+diff --git a/chrome/browser/autocomplete/autocomplete_classifier_factory.cc b/chrome/browser/autocomplete/autocomplete_classifier_factory.cc
+index db73283fbc4f5146bb42e2a8abfcdfa4567ab252..f241c6c5f0611dcbcad3e468a7595be75324a8ba 100644
+--- a/chrome/browser/autocomplete/autocomplete_classifier_factory.cc
++++ b/chrome/browser/autocomplete/autocomplete_classifier_factory.cc
+@@ -16,6 +16,7 @@
+ #include "components/omnibox/browser/autocomplete_classifier.h"
+ #include "components/omnibox/browser/autocomplete_controller.h"
+ #include "extensions/buildflags/buildflags.h"
++#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"
+
+ #if BUILDFLAG(ENABLE_EXTENSIONS)
+ #include "extensions/browser/extension_system_provider.h"
+@@ -43,7 +44,7 @@ std::unique_ptr<KeyedService> AutocompleteClassifierFactory::BuildInstanceFor(
+       std::make_unique<AutocompleteController>(
+           std::make_unique<ChromeAutocompleteProviderClient>(profile),
+           AutocompleteClassifier::DefaultOmniboxProviders()),
+-      std::make_unique<ChromeAutocompleteSchemeClassifier>(profile));
++      std::make_unique<BraveAutocompleteSchemeClassifier>(profile));
+ }
+
+ AutocompleteClassifierFactory::AutocompleteClassifierFactory()
```

The produced patch is supposed to be committed in the repository as usual.

### Checking if files are up-to-date

You can verify if all plaster files are applied, with Chromium sources updated
and patch files checked under `path` with the following command:

```sh
tools/cr/plaster.py check
```

This is the equivalent of a dry run of `plaster.py apply`.
