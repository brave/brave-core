/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_location_bar_model_delegate.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/common/url_constants.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#endif

BraveLocationBarModelDelegate::BraveLocationBarModelDelegate(Browser* browser)
    : BrowserLocationBarModelDelegate(browser) {}

BraveLocationBarModelDelegate::~BraveLocationBarModelDelegate() {}

// static
void BraveLocationBarModelDelegate::FormattedStringFromURL(
    const GURL& url,
    std::u16string* new_formatted_url) {
  if (url.SchemeIs("chrome")) {
    base::ReplaceFirstSubstringAfterOffset(new_formatted_url, 0, u"chrome://",
                                           u"brave://");
  }

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  if (url.SchemeIs(kChromeExtensionScheme) &&
      url.host() == ethereum_remote_client_extension_id) {
    base::ReplaceFirstSubstringAfterOffset(
        new_formatted_url, 0,
        base::UTF8ToUTF16(ethereum_remote_client_base_url), u"brave://wallet");
    base::ReplaceFirstSubstringAfterOffset(
        new_formatted_url, 0,
        base::UTF8ToUTF16(ethereum_remote_client_phishing_url),
        u"brave://wallet");
    base::ReplaceFirstSubstringAfterOffset(
        new_formatted_url, 0,
        base::UTF8ToUTF16(ethereum_remote_client_ens_redirect_url),
        u"brave://wallet");
  }
#endif
}

std::u16string
BraveLocationBarModelDelegate::FormattedStringWithEquivalentMeaning(
    const GURL& url,
    const std::u16string& formatted_url) const {
  std::u16string new_formatted_url =
      BrowserLocationBarModelDelegate::FormattedStringWithEquivalentMeaning(
          url, formatted_url);
  BraveLocationBarModelDelegate::FormattedStringFromURL(url,
                                                        &new_formatted_url);
  return new_formatted_url;
}
