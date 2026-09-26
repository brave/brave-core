/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PLAYLIST_PLAYLIST_DATA_SOURCE_H_
#define BRAVE_BROWSER_PLAYLIST_PLAYLIST_DATA_SOURCE_H_

#include <string>

#include "base/files/file_path.h"
#include "chrome/browser/ui/webui/favicon_source.h"

class GURL;
class Profile;

namespace playlist {

class PlaylistService;

// A URL data source for
// chrome-untrusted://playlist-data/<playlist-id>/{thumbnail,media,favicon}/
// resources, for use in webui pages that want to get thumbnails or media data.
//
// Streams saved as HLS are served under
// chrome-untrusted://playlist-data/<playlist-id>/hls/<file>, where <file> is
// relative to the item's "hls" subdirectory. Chromium's built-in HLS demuxer
// selects itself based on the ".m3u8" suffix, then fetches the segments the
// local manifest names through this same route.
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
      kNone,
      kThumbnail,
      kMedia,
      kFavicon,
      kHls,
    };

    explicit DataRequest(const GURL& url);
    DataRequest(const DataRequest&) = delete;
    DataRequest& operator=(const DataRequest&) = delete;
    ~DataRequest();

    std::string id;
    Type type = Type::kNone;
    // For `kHls` only: the requested file, relative to the item's "hls"
    // subdirectory.
    base::FilePath hls_file;
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
  void GetHlsManifest(const DataRequest& request,
                      GotDataCallback got_data_callback);
  void GetHlsSegment(const DataRequest& request,
                     const net::HttpByteRange& range,
                     GotRangeDataCallback got_data_callback);
  // Returns the absolute path for a `kHls` request, or an empty path if the
  // item is unknown or the request escapes the item's directory.
  base::FilePath ResolveHlsPath(const DataRequest& request) const;

  raw_ptr<PlaylistService, DanglingUntriaged> service_ = nullptr;
};

}  // namespace playlist

#endif  // BRAVE_BROWSER_PLAYLIST_PLAYLIST_DATA_SOURCE_H_
