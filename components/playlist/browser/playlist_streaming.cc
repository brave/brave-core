/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_streaming.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/task/thread_pool.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace playlist {

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTagForURLLoad() {
  return net::DefineNetworkTrafficAnnotation("playlist_thumbnail_downloader",
                                             R"(
      semantics {
        sender: "Brave playlist thumbnail downloader"
        description:
          "Fetching thumbnail image for newly created playlist item"
        trigger:
          "User-initiated for creating new playlist item"
        data:
          "Thumbnail for playlist item"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
      })");
}

}  // namespace

PlaylistStreaming::PlaylistStreaming(content::BrowserContext* context)
    : url_loader_factory_(context->GetDefaultStoragePartition()
                              ->GetURLLoaderFactoryForBrowserProcess()),
      api_request_helper_(
          std::make_unique<api_request_helper::APIRequestHelper>(
              GetNetworkTrafficAnnotationTagForURLLoad(),
              url_loader_factory_)) {}

PlaylistStreaming::~PlaylistStreaming() = default;

void PlaylistStreaming::QueryPrompt(
    const std::string& url,
    const std::string& method,
    api_request_helper::APIRequestHelper::ResponseStartedCallback
        response_started_callback,
    api_request_helper::APIRequestHelper::DataReceivedCallback
        data_received_callback,
    api_request_helper::APIRequestHelper::ResultCallback
        data_completed_callback) {
  LOG(ERROR) << "data_source : "
             << "PlaylistStreaming::QueryPrompt";
  base::flat_map<std::string, std::string> headers;
  // headers.emplace("Range", "bytes=0-4000");
  LOG(ERROR) << "data_source"
             << "PlaylistStreaming : url : " << url << " method : " << method;
  api_request_helper_->RequestSSE(method, GURL(url), "", "application/json",
                                  std::move(data_received_callback),
                                  std::move(data_completed_callback), headers,
                                  {}, std::move(response_started_callback));
}

}  // namespace playlist
