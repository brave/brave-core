/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_PROVIDER_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_PROVIDER_H_

#include <string>

#include "extensions/browser/management_policy.h"

namespace extensions {

class BraveExtensionProvider : public ManagementPolicy::Provider {
 public:
  BraveExtensionProvider();
  BraveExtensionProvider(const BraveExtensionProvider&) = delete;
  BraveExtensionProvider& operator=(const BraveExtensionProvider&) = delete;
  ~BraveExtensionProvider() override;
  std::string GetDebugPolicyProviderName() const override;
  bool UserMayLoad(const Extension* extension,
                   std::u16string* error) const override;
  bool MustRemainInstalled(const Extension* extension,
                           std::u16string* error) const override;
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_PROVIDER_H_
