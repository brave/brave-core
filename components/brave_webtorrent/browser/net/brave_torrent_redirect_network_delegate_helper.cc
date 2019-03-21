/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_webtorrent/browser/net/brave_torrent_redirect_network_delegate_helper.h"

#include <memory>
#include <string>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/browser/renderer_host/brave_navigation_ui_data.h"
#include "brave/common/network_constants.h"
#include "brave/common/extensions/extension_constants.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/info_map.h"
#include "extensions/common/constants.h"
#include "net/http/http_content_disposition.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"

namespace {

bool FileNameMatched(const net::HttpResponseHeaders* headers) {
  std::string disposition;
  if (!headers->GetNormalizedHeader("Content-Disposition", &disposition)) {
    return false;
  }

  net::HttpContentDisposition cd_headers(disposition, std::string());
  if (base::EndsWith(cd_headers.filename(), ".torrent",
        base::CompareCase::INSENSITIVE_ASCII) ||
      base::EndsWith(cd_headers.filename(), ".torrent\"",
        base::CompareCase::INSENSITIVE_ASCII)) {
    return true;
  }

  return false;
}

bool URLMatched(net::URLRequest* request) {
  return base::EndsWith(request->url().spec(), ".torrent",
      base::CompareCase::INSENSITIVE_ASCII);
}

bool IsTorrentFile(net::URLRequest* request,
    const net::HttpResponseHeaders* headers) {
  std::string mimeType;
  if (!headers->GetMimeType(&mimeType)) {
    return false;
  }

  if (mimeType == kBittorrentMimeType) {
    return true;
  }

  if (mimeType == kOctetStreamMimeType &&
      (URLMatched(request) || FileNameMatched(headers))) {
    return true;
  }

  return false;
}

bool IsWebtorrentInitiated(net::URLRequest* request) {
  return request->initiator().has_value() &&
    request->initiator()->GetURL().spec() ==
      base::StrCat({extensions::kExtensionScheme, "://",
      brave_webtorrent_extension_id, "/"});
}

bool IsTorProfile(net::URLRequest* request) {
  content::ResourceRequestInfo* resource_info =
    content::ResourceRequestInfo::ForRequest(request);
  if (!resource_info) {
    return false;
  }

  const BraveNavigationUIData* ui_data =
    static_cast<const BraveNavigationUIData*>(
        resource_info->GetNavigationUIData());
  if (!ui_data) {
    return false;
  }

  return ui_data->GetTorProfileService() ? true : false;
}

bool IsWebTorrentDisabled(net::URLRequest* request) {
  content::ResourceRequestInfo* resource_info =
    content::ResourceRequestInfo::ForRequest(request);
  if (!resource_info || !resource_info->GetContext()) {
    return false;
  }

  const ProfileIOData* io_data =
    ProfileIOData::FromResourceContext(resource_info->GetContext());
  if (!io_data) {
    return false;
  }

  const extensions::InfoMap* infoMap = io_data->GetExtensionInfoMap();
  if (!infoMap) {
    return false;
  }

  return !infoMap->extensions().Contains(brave_webtorrent_extension_id) ||
    infoMap->disabled_extensions().Contains(brave_webtorrent_extension_id);
}

}  // namespace

namespace webtorrent {

int OnHeadersReceived_TorrentRedirectWork(
    net::URLRequest* request,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url,
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {

  if (!request || !original_response_headers ||
      IsTorProfile(request) ||
      IsWebTorrentDisabled(request) ||
      IsWebtorrentInitiated(request) ||  // download .torrent, do not redirect
      !IsTorrentFile(request, original_response_headers)) {
    return net::OK;
  }

  *override_response_headers =
    new net::HttpResponseHeaders(original_response_headers->raw_headers());
  (*override_response_headers)
      ->ReplaceStatusLine("HTTP/1.1 307 Temporary Redirect");
  (*override_response_headers)->RemoveHeader("Location");
  GURL url(
      base::StrCat({extensions::kExtensionScheme, "://",
      brave_webtorrent_extension_id,
      "/extension/brave_webtorrent.html?",
      request->url().spec()}));
  (*override_response_headers)->AddHeader(
      "Location: " + url.spec());
  *allowed_unsafe_redirect_url = url;
  return net::OK;
}

}  // namespace webtorrent
