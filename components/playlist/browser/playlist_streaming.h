/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_STREAMING_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_STREAMING_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace playlist {

// This class is key interface for HLS streaming on android playlist.
// Playlist stream request is started by calling
// RequestStreamingQuery from client.
// Different callbacks would notify client the status.
class PlaylistStreaming {
 public:
  explicit PlaylistStreaming(content::BrowserContext* context);
  ~PlaylistStreaming();
  PlaylistStreaming(const PlaylistStreaming&) = delete;
  PlaylistStreaming& operator=(const PlaylistStreaming&) = delete;

  void RequestStreamingQuery(
      const std::string& query_id,
      const std::string& url,
      const std::string& method,
      api_request_helper::APIRequestHelper::ResponseStartedCallback
          response_started_callback,
      api_request_helper::APIRequestHelper::DataReceivedCallback
          data_received_callback,
      api_request_helper::APIRequestHelper::ResultCallback
          data_completed_callback);
  void ClearAllQueries();
  void CancelQuery(const std::string& query_id);

 private:
  using URLLoaderMap =
      base::flat_map<std::string, api_request_helper::APIRequestHelper::Ticket>;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  URLLoaderMap url_loader_map_;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_STREAMING_H_
