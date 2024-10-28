/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PLAYLIST_PLAYLIST_DATA_SOURCE_H_
#define BRAVE_BROWSER_PLAYLIST_PLAYLIST_DATA_SOURCE_H_

#include <string>

#include "chrome/browser/ui/webui/favicon_source.h"

class GURL;
class Profile;

namespace playlist {

class PlaylistService;

// A URL data source for
// chrome-untrusted://playlist-data/<playlist-id>/{thumbnail,media,favicon}/
// resources, for use in webui pages that want to get thumbnails or media data.
class PlaylistDataSource : public FaviconSource {
 public:
  PlaylistDataSource(Profile* profile, PlaylistService* service);
  PlaylistDataSource(const PlaylistDataSource&) = delete;
  PlaylistDataSource& operator=(const PlaylistDataSource&) = delete;
  ~PlaylistDataSource() override;

  // content::URLDataSource implementation.
  std::string GetSource() override;
  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback got_data_callback) override;
  void StartRangeDataRequest(const GURL& url,
                             const content::WebContents::Getter& wc_getter,
                             const net::HttpByteRange& range,
                             GotRangeDataCallback callback) override;
  std::string GetMimeType(const GURL& url) override;
  bool AllowCaching() override;
  bool SupportsRangeRequests(const GURL& url) const override;

 private:
  struct DataRequest {
    enum class Type {
      kThumbnail,
      kMedia,
      kFavicon,
    };

    explicit DataRequest(const GURL& url);
    DataRequest(const DataRequest&) = delete;
    DataRequest& operator=(const DataRequest&) = delete;
    ~DataRequest();

    std::string id;
    Type type;
  };

  void GetThumbnail(const DataRequest& request,
                    const content::WebContents::Getter& wc_getter,
                    GotDataCallback got_data_callback);
  void GetFavicon(const DataRequest& request,
                  const content::WebContents::Getter& wc_getter,
                  GotDataCallback got_data_callback);
  void GetMediaFile(const DataRequest& request,
                    const content::WebContents::Getter& wc_getter,
                    const net::HttpByteRange& range,
                    GotRangeDataCallback got_data_callback);

  raw_ptr<PlaylistService, DanglingUntriaged> service_ = nullptr;
};

}  // namespace playlist

#endif  // BRAVE_BROWSER_PLAYLIST_PLAYLIST_DATA_SOURCE_H_
