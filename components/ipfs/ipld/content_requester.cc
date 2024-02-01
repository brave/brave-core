/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/content_requester.h"

#include "base/logging.h"
#include "brave/components/ipfs/ipfs_network_utils.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace {

constexpr int64_t kMaxFileSizeInBytes = 8 * 1024 * 1024;  // 8 MB

bool IsPublicGatewayLink(const GURL& ipfs_url, PrefService* prefs) {
  if (ipfs::IsIPFSScheme(ipfs_url)) {
    return false;
  }
  if (ipfs::IsLocalGatewayURL(ipfs_url)) {
    return false;
  }
  return ipfs::IsDefaultGatewayURL(ipfs_url, prefs);
}

}  // namespace

namespace ipfs::ipld {

ContentRequester::ContentRequester(
    const GURL& url,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs)
    : url_(url), prefs_(prefs), url_loader_factory_(url_loader_factory) {}

ContentRequester::~ContentRequester() = default;

bool ContentRequester::IsStarted() const {
  return is_started_;
}

void ContentRequester::Start() {
  auto gateway_request_url = GetGatewayRequestUrl();

  if (gateway_request_url.is_empty()) {
    return;
  }

  is_started_ = true;

  auto simple_url_loader = CreateURLLoader(gateway_request_url, "GET");

  simple_url_loader->DownloadToTempFile(
      url_loader_factory_.get(),
      base::BindOnce(&ContentRequester::OnUrlDownloadedToTempFile,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(simple_url_loader)
                     // std::move(download_job)
                     ),
      kMaxFileSizeInBytes);
}

const GURL ContentRequester::GetGatewayRequestUrl() const {
  if (url_.is_empty()) {
    return {};
  }

  GURL url_res(url_);
  if (!IsPublicGatewayLink(url_, prefs_)) {
    std::string cid, ipfs_path;
    ParseCIDAndPathFromIPFSUrl(url_, &cid, &ipfs_path);
    url_res = ipfs::ToPublicGatewayURL(url_, prefs_);
  }

  LOG(INFO) << "[IPFS] ContentRequester::GetGatewayRequestUrl() url_res:"
            << url_res;
  return url_res;
}

std::unique_ptr<network::ResourceRequest> ContentRequester::RequestContent(
    const GURL& url) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;

  return request;
}

void ContentRequester::OnUrlDownloadedToTempFile(
    std::unique_ptr<network::SimpleURLLoader> simple_loader,
    //    std::unique_ptr<Job> download_job,
    base::FilePath temp_path) {
  LOG(INFO) << "[IPFS] OnUrlDownloadedToTempFile: " << temp_path;
}

}  // namespace ipfs::ipld
