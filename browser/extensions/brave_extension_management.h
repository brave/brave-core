/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_EXTENSION_MANAGEMENT_H_
#define BRAVE_BROWSER_EXTENSIONS_EXTENSION_MANAGEMENT_H_

#include "chrome/browser/extensions/extension_management.h"

namespace extensions {

class BraveExtensionManagement : public ExtensionManagement {
 public:
  BraveExtensionManagement(PrefService* pref_service, bool is_signin_profile);
  ~BraveExtensionManagement() override;

 private:
  void RegisterForceInstalledExtensions();
  void RegisterBraveExtensions();
  DISALLOW_COPY_AND_ASSIGN(BraveExtensionManagement);
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_EXTENSION_MANAGEMENT_H_
