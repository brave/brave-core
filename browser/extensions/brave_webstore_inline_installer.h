/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_WEBSTORE_INLINE_INSTALLER_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_WEBSTORE_INLINE_INSTALLER_H_

class BraveWebstoreBrowserTest;

#include "chrome/browser/extensions/webstore_inline_installer.h"

namespace extensions {

extern const char kWebstoreUrlFormat[];

class BraveWebstoreInlineInstaller : public WebstoreInlineInstaller {
 public:
  BraveWebstoreInlineInstaller(content::WebContents* web_contents,
                               content::RenderFrameHost* host,
                               const std::string& webstore_item_id,
                               const GURL& requestor_url,
                               const Callback& callback);

 protected:
  ~BraveWebstoreInlineInstaller() override;
  friend class ::BraveWebstoreBrowserTest;
  bool CheckInlineInstallPermitted(const base::DictionaryValue& webstore_data,
                                   std::string* error) const override;
  DISALLOW_IMPLICIT_CONSTRUCTORS(BraveWebstoreInlineInstaller);
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_WEBSTORE_INLINE_INSTALLER_H_
