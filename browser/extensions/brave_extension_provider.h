/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_EXTENSIONS_BROWSER_MANAGEMENT_POLICY_H_
#define BRAVE_EXTENSIONS_BROWSER_MANAGEMENT_POLICY_H_

#include "extensions/browser/management_policy.h"

namespace extensions {

class BraveExtensionProvider : public ManagementPolicy::Provider {
 public:
  BraveExtensionProvider();
  ~BraveExtensionProvider() override;
  std::string GetDebugPolicyProviderName() const override;
  bool UserMayLoad(const Extension* extension,
                   base::string16* error) const override;
  bool MustRemainInstalled(const Extension* extension,
                           base::string16* error) const override;
  static bool IsVetted(const extensions::Extension* extension);
 private:
  DISALLOW_COPY_AND_ASSIGN(BraveExtensionProvider);
};

}  // namespace extensions

#endif
