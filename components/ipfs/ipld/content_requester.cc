/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/content_requester.h"
#include <cstdint>
#include <cstdlib>
#include <memory>

#include "base/logging.h"
#include "brave/components/ipfs/ipfs_network_utils.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/ipld/car_content_requester.h"
#include "net/http/http_status_code.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace {

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

std::unique_ptr<ContentRequester>
ContentRequesterFactory::CreateCarContentRequester(
    const GURL& url,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs,
    const bool only_structure) {
  return std::make_unique<CarContentRequester>(url, url_loader_factory, prefs,
                                               only_structure);
}

ContentRequester::ContentRequester(
    const GURL& url,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs)
    : url_(url), prefs_(prefs), url_loader_factory_(url_loader_factory) {}

ContentRequester::~ContentRequester() = default;

bool ContentRequester::IsStarted() const {
  return is_started_;
}

void ContentRequester::Reset(const GURL& new_url) {
  LOG(INFO) << "[IPFS] Reset #10";
  url_ = new_url;
  is_started_ = false;
  url_loader_.release();  
}

void ContentRequester::Request(ContentRequestBufferCallback callback) {
  if (GetGatewayRequestUrl().is_empty()) {
    return;
  }
  LOG(INFO) << "[IPFS] Request #10";
  buffer_ready_callback_ = std::move(callback);
  url_loader_ = CreateLoader();
  url_loader_->DownloadAsStream(url_loader_factory_.get(), this);
  LOG(INFO) << "[IPFS] Request #20";
  is_started_ = true;
}

GURL ContentRequester::GetGatewayRequestUrl() const {
  if (url_.is_empty()) {
    return {};
  }

  GURL url_res(url_);
  if (!IsPublicGatewayLink(url_, prefs_)) {
    std::string cid;
    std::string ipfs_path;
    ParseCIDAndPathFromIPFSUrl(url_, &cid, &ipfs_path);
    url_res = ipfs::ToPublicGatewayURL(url_, prefs_);
  }

  LOG(INFO) << "[IPFS] ContentRequester::GetGatewayRequestUrl() url_res:"
            << url_res;
  return url_res;
}

void ContentRequester::OnDataReceived(base::StringPiece string_piece,
                                      base::OnceClosure resume) {
  LOG(INFO) << "[IPFS] OnDataReceived #10";
  auto data = std::make_unique<std::vector<uint8_t>>(string_piece.begin(),
                                                     string_piece.end());

  LOG(INFO) << "[IPFS] OnDataReceived bytes_received_:" << data->size(); //<< "\r\n" << string_piece;

  if (buffer_ready_callback_) {
    buffer_ready_callback_.Run(std::move(data), false, net::HTTP_OK);
  }

  if (!resume) {
    return;
  }

  // Continue to read data.
  std::move(resume).Run();
}

void ContentRequester::OnRetry(base::OnceClosure start_retry) {
  // TODO Decide should we use retry or no
}

void ContentRequester::OnComplete(bool success) {
  LOG(INFO) << "[IPFS] OnComplete success:" << success; // << " err:" << url_loader_->NetError()
  // << "\r\n url_loader_:" << (url_loader_ != nullptr)
  // << "\r\n url_loader_->ResponseInfo():" << (url_loader_ != nullptr && url_loader_->ResponseInfo())
  // << "\r\n url_loader_->ResponseInfo()->headers:" << (url_loader_ != nullptr && url_loader_->ResponseInfo() && url_loader_->ResponseInfo()->headers)
  ;

  int response_code = url_loader_ ? url_loader_->NetError() : net::Error::ERR_HTTP_RESPONSE_CODE_FAILURE;
  if (url_loader_ && url_loader_->ResponseInfo() && url_loader_->ResponseInfo()->headers) {
    response_code = url_loader_->ResponseInfo()->headers->response_code();

    LOG(INFO) << "[IPFS] OnComplete response_code:" << response_code;
  }

  if (buffer_ready_callback_) {
    buffer_ready_callback_.Run(nullptr, true, response_code);
  }

  is_started_ = false;
  //url_loader_.release();
}
}  // namespace ipfs::ipld
