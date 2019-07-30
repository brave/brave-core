/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_webtorrent/browser/net/brave_torrent_redirect_network_delegate_helper.h"

#include <memory>
#include <string>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/network_constants.h"
#include "content/public/common/resource_type.h"
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

bool URLMatched(const GURL& url) {
  return base::EndsWith(url.spec(), ".torrent",
      base::CompareCase::INSENSITIVE_ASCII);
}

/**
 * Returns true if the URL contains a URL fragment that starts with "ix=". For
 * example, https://webtorrent.io/torrents/big-buck-bunny.torrent#ix=1.
 * Otherwise, returns false.
 */
bool IsViewerURL(const GURL& url) {
  return base::StartsWith(url.ref(), "ix=",
      base::CompareCase::INSENSITIVE_ASCII);
}

bool IsTorrentFile(const GURL& url, const net::HttpResponseHeaders* headers) {
  std::string mimeType;
  if (!headers->GetMimeType(&mimeType)) {
    return false;
  }

  if (mimeType == kBittorrentMimeType) {
    return true;
  }

  if (mimeType == kOctetStreamMimeType &&
      (URLMatched(url) || FileNameMatched(headers))) {
    return true;
  }

  return false;
}

bool IsWebtorrentInitiated(std::shared_ptr<brave::BraveRequestInfo> ctx) {
  return ctx->initiator_url.scheme() == extensions::kExtensionScheme &&
      ctx->initiator_url.host() == brave_webtorrent_extension_id;
}

/**
 * Returns true if the resource type is a frame (i.e. a top level page) or a
 * subframe (i.e. a frame or iframe). For all other resource types (stylesheet,
 * script, XHR request, etc.), returns false.
 */

bool IsFrameResource(std::shared_ptr<brave::BraveRequestInfo> ctx) {
  return content::IsResourceTypeFrame(ctx->resource_type);
}

}  // namespace

namespace webtorrent {

int OnHeadersReceived_TorrentRedirectWork(
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url,
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  if (!original_response_headers ||
      !IsFrameResource(ctx) ||
      ctx->is_webtorrent_disabled ||
      // download .torrent, do not redirect
      (IsWebtorrentInitiated(ctx) && !IsViewerURL(ctx->request_url)) ||
      !IsTorrentFile(ctx->request_url, original_response_headers)) {
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
      ctx->request_url.spec()}));
  (*override_response_headers)->AddHeader("Location: " + url.spec());
  *allowed_unsafe_redirect_url = url;
  return net::OK;
}

}  // namespace webtorrent
