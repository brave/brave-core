/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_webtorrent/browser/webtorrent_util.h"

#include <string>

#include "base/strings/string_util.h"
#include "brave/components/constants/network_constants.h"
#include "brave/components/constants/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "net/http/http_content_disposition.h"
#include "net/http/http_response_headers.h"

namespace webtorrent {

bool TorrentFileNameMatched(const net::HttpResponseHeaders* headers) {
  std::optional<std::string> disposition =
      headers->GetNormalizedHeader("Content-Disposition");
  if (!disposition) {
    return false;
  }

  net::HttpContentDisposition cd_headers(*disposition, std::string());
  if (base::EndsWith(cd_headers.filename(), ".torrent",
        base::CompareCase::INSENSITIVE_ASCII) ||
      base::EndsWith(cd_headers.filename(), ".torrent\"",
        base::CompareCase::INSENSITIVE_ASCII)) {
    return true;
  }

  return false;
}

bool TorrentURLMatched(const GURL& url) {
  return base::EndsWith(url.path(), ".torrent",
      base::CompareCase::INSENSITIVE_ASCII);
}

bool IsWebtorrentEnabled(content::BrowserContext* browser_context) {
  extensions::ExtensionRegistry* registry =
    extensions::ExtensionRegistry::Get(browser_context);
  if (!registry) {
    return false;
  }
  return registry->enabled_extensions().Contains(brave_webtorrent_extension_id);
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kWebTorrentEnabled, true);
}

bool IsWebtorrentURL(const GURL& url) {
  if (url.SchemeIs(extensions::kExtensionScheme) &&
      url.host() == brave_webtorrent_extension_id &&
      (url.ExtractFileName() == brave_webtorrent_extension_filename ||
       url.ExtractFileName() == brave_webtorrent_extension_filename2)) {
    return true;
  }

  return false;
}

bool IsTorrentFile(const GURL& url, const net::HttpResponseHeaders* headers) {
  if (!headers) {
    return false;
  }

  std::string mimeType;
  if (!headers->GetMimeType(&mimeType)) {
    return false;
  }

  if (mimeType == kBittorrentMimeType) {
    return true;
  }

  if (mimeType == kOctetStreamMimeType &&
      (TorrentURLMatched(url) || TorrentFileNameMatched(headers))) {
    return true;
  }

  return false;
}

}  // namespace webtorrent
