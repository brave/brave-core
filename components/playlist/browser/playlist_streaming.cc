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
  return net::DefineNetworkTrafficAnnotation("playlist_streaming",
                                             R"(
      semantics {
        sender: "Brave playlist streaming"
        description:
          "Fetching the playlist content"
        trigger:
          "User-initiated for steraming playlist content"
        data:
          "Playlist data for playlist item"
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

void PlaylistStreaming::RequestStreamingQuery(
    const std::string& query_id,
    const std::string& url,
    const std::string& method,
    api_request_helper::APIRequestHelper::ResponseStartedCallback
        response_started_callback,
    api_request_helper::APIRequestHelper::DataReceivedCallback
        data_received_callback,
    api_request_helper::APIRequestHelper::ResultCallback
        data_completed_callback) {
  base::flat_map<std::string, std::string> headers;
  api_request_helper::APIRequestHelper::Ticket ticket =
      api_request_helper_->RequestSSE(
          method, GURL(url), "", "application/json",
          std::move(data_received_callback), std::move(data_completed_callback),
          headers, {}, std::move(response_started_callback));
  url_loader_map_[query_id] = ticket;
}

void PlaylistStreaming::ClearAllQueries() {
  api_request_helper_->CancelAll();
  url_loader_map_.clear();
}

void PlaylistStreaming::CancelQuery(const std::string& query_id) {
  if (!url_loader_map_.count(query_id)) {
    return;
  }
  api_request_helper::APIRequestHelper::Ticket ticket =
      url_loader_map_[query_id];
  api_request_helper_->Cancel(ticket);
  url_loader_map_.erase(query_id);
}

}  // namespace playlist
