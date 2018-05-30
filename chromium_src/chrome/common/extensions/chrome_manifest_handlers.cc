/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define RegisterChromeManifestHandlers RegisterChromeManifestHandlers_INTERNAL
#include "../../../../../chrome/common/extensions/chrome_manifest_handlers.cc"
#undef RegisterChromeManifestHandlers

#include "brave/common/extensions/manifest_handlers/pdfjs_manifest_override.h"

namespace extensions {

void RegisterChromeManifestHandlers() {
  RegisterChromeManifestHandlers_INTERNAL();
  (new PDFJSOverridesHandler)->Register();
}

}  // namespace extensions
