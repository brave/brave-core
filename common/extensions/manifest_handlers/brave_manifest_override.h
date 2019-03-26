/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_EXTENSIONS_MANIFEST_HANDLERS_BRAVE_OVERRIDES_HANDLER_H_
#define BRAVE_COMMON_EXTENSIONS_MANIFEST_HANDLERS_BRAVE_OVERRIDES_HANDLER_H_

#include "chrome/common/extensions/manifest_handlers/extension_action_handler.h"

namespace extensions {

// This needs to be a subclass because you can't have 2 manifest handlers
// for the same keys.
class BraveOverridesHandler : public ExtensionActionHandler {
 public:
  BraveOverridesHandler();
  ~BraveOverridesHandler() override;
  bool Parse(Extension* extension, base::string16* error) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveOverridesHandler);
};

}  // namespace extensions
#endif  // BRAVE_COMMON_EXTENSIONS_MANIFEST_HANDLERS_BRAVE_OVERRIDES_HANDLER_H_
