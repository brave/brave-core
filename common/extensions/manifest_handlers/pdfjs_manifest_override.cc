/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/extensions/extension_constants.h"
#include "brave/common/extensions/manifest_handlers/pdfjs_manifest_override.h"
#include "chrome/common/extensions/api/extension_action/action_info.h"
#include "extensions/common/extension.h"

namespace extensions {

PDFJSOverridesHandler::PDFJSOverridesHandler() {
}

PDFJSOverridesHandler::~PDFJSOverridesHandler() {
}

bool PDFJSOverridesHandler::Parse(Extension* extension, base::string16* error) {
  bool result = ExtensionActionHandler::Parse(extension, error);
  if (extension && extension->id() == pdfjs_extension_id) {
    ActionInfo::SetPageActionInfo(extension, nullptr);
  }
  return result;
}

}  // namespace extensions
