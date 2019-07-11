/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_EXTENSION_CONVERTER_H_
#define BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_EXTENSION_CONVERTER_H_

#include <string>

#include "base/memory/ref_counted.h"

class GURL;

namespace base {
class FilePath;
}

namespace extensions {
class Extension;
}

namespace greaselion {

class GreaselionRule;

// Wraps a Greaselion rule in a component. The component is stored as an
// unpacked extension in the system temp dir. Returns a valid extension that the
// caller should take ownership of, or NULL.
//
// NOTE: This function does file IO and should not be called on the UI thread.
// NOTE: The caller takes ownership of the directory at extension->path() on the
// returned object.
scoped_refptr<extensions::Extension>
ConvertGreaselionRuleToExtensionOnTaskRunner(
    const GreaselionRule& rule,
    const base::FilePath& extensions_dir);

}  // namespace greaselion

#endif  // BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_EXTENSION_CONVERTER_H_
