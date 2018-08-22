/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_webstore_inline_installer.h"

#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "content/public/browser/web_contents.h"

namespace extensions {

// The URL to the webstore page for a specific app.
const char kWebstoreUrlFormat[] =
    "https://chrome.google.com/webstore/detail/%s";

BraveWebstoreInlineInstaller::BraveWebstoreInlineInstaller(
    content::WebContents* web_contents,
    content::RenderFrameHost* host,
    const std::string& webstore_item_id,
    const GURL& requestor_url,
    const Callback& callback)
    : WebstoreInlineInstaller(web_contents, host,
        webstore_item_id, requestor_url, callback) {
}

BraveWebstoreInlineInstaller::~BraveWebstoreInlineInstaller() {}

bool BraveWebstoreInlineInstaller::CheckInlineInstallPermitted(
    const base::DictionaryValue& webstore_data,
    std::string* error) const {
  // Open a URL corresponding to the extension ID.
  GURL url(base::StringPrintf(kWebstoreUrlFormat, id().c_str()));
  web_contents()->OpenURL(content::OpenURLParams(
      url, content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui::PAGE_TRANSITION_LINK, false));
  // Return an error so nothing else is processed
  *error = kInlineInstallNotSupportedKey;
  return false;
}

}  // namespace extensions
