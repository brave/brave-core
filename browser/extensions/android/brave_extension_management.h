/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_ANDROID_BRAVE_EXTENSION_MANAGEMENT_H_
#define BRAVE_BROWSER_EXTENSIONS_ANDROID_BRAVE_EXTENSION_MANAGEMENT_H_

#include "chrome/browser/extensions/extension_management.h"

class Profile;

namespace extensions {

// Whether non-component extensions may be installed and run.
bool AreAndroidExtensionsAllowed();

class BraveExtensionManagement : public ExtensionManagement {
 public:
  explicit BraveExtensionManagement(Profile* profile);
  BraveExtensionManagement(const BraveExtensionManagement&) = delete;
  BraveExtensionManagement& operator=(const BraveExtensionManagement&) = delete;
  ~BraveExtensionManagement() override;
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_ANDROID_BRAVE_EXTENSION_MANAGEMENT_H_
