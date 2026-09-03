/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_STREAM_DOWNLOADER_H_
#define BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_STREAM_DOWNLOADER_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/playlist/content/browser/playlist_dash_parser.h"
#include "brave/components/playlist/content/browser/playlist_manifest_writer.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace playlist {

// Downloads a stream in full and repackages it for local playback: segments
// are stored verbatim and a local HLS manifest naming them is written
// alongside. Nothing is decoded, muxed or re-encoded.
//
// Handles both HLS and DASH sources. DASH is converted to a local HLS manifest
// rather than kept as an MPD, because Chromium ships an HLS demuxer and no
// DASH one.
//
// When a stream carries its audio as a separate rendition, both renditions are
// saved and a local multivariant manifest binds them back together - the HLS
// demuxer syncs them at playback, which is what lets us support streams that
// nothing in the tree could mux.
//
// One instance handles one stream. Destroying it cancels everything in flight.
class PlaylistStreamDownloader {
 public:
  enum class Error {
    kNetwork,
    kParse,
    // Encrypted streams are recognized and refused rather than saved as
    // undecryptable bytes.
    kEncrypted,
    kNoPlayableRendition,
    kFileSystem,
  };

  struct Result {
    // The local manifest to hand to a media element, relative to the
    // destination directory.
    std::string manifest_file_name;
    int64_t received_bytes = 0;
  };

  using ResultCallback =
      base::OnceCallback<void(base::expected<Result, Error>)>;
  using ProgressCallback = base::RepeatingCallback<void(int64_t received_bytes,
                                                        int percent_complete)>;

  explicit PlaylistStreamDownloader(content::BrowserContext* context);
  PlaylistStreamDownloader(const PlaylistStreamDownloader&) = delete;
  PlaylistStreamDownloader& operator=(const PlaylistStreamDownloader&) = delete;
  ~PlaylistStreamDownloader();

  // `destination_dir` must already exist.
  void Start(const GURL& manifest_url,
             const base::FilePath& destination_dir,
             ProgressCallback on_progress,
             ResultCallback on_result);

  static std::string_view ErrorToString(Error error);

 private:
  // One remote file to save locally, plus the name to save it under.
  struct PendingFile {
    GURL url;
    std::string file_name;
  };

  // A rendition being assembled: what to fetch, and what to write.
  struct RenditionJob {
    std::string playlist_file_name;
    std::vector<PendingFile> files;
    HlsRendition manifest;
  };

  void FetchManifest(const GURL& url,
                     base::OnceCallback<void(std::optional<std::string>)>);
  void OnManifestFetched(
      network::SimpleURLLoader* loader,
      base::OnceCallback<void(std::optional<std::string>)> callback,
      std::optional<std::string> body);
  void OnRootManifestFetched(std::optional<std::string> body);
  void OnDashManifestParsed(
      base::expected<DashManifest, DashParseError> parsed);
  void OnMediaPlaylistFetched(size_t rendition_index,
                              std::optional<std::string> body);

  // Returns false if the playlist is unusable, having already reported why.
  bool BuildRenditionJob(size_t rendition_index, std::string_view body);

  void MaybeStartSegmentDownloads();
  void StartNextSegmentDownload();
  void OnSegmentDownloaded(size_t rendition_index,
                           size_t file_index,
                           network::SimpleURLLoader* loader,
                           base::FilePath path);

  void WriteManifests();
  void OnManifestsWritten(bool success);

  void Fail(Error error);
  void Succeed(const std::string& manifest_file_name);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  GURL manifest_url_;
  base::FilePath destination_dir_;
  ProgressCallback on_progress_;
  ResultCallback on_result_;

  // Index 0 is always the primary (video, or muxed) rendition. Index 1, when
  // present, is a separate audio rendition.
  std::vector<RenditionJob> renditions_;
  // Set when the source was a multivariant playlist with a separate audio
  // rendition, in which case a local multivariant manifest is written too.
  std::optional<HlsMultivariant> multivariant_;

  // The local manifest a media element should be pointed at, decided once the
  // renditions are known and consumed after they're written.
  std::string pending_entry_point_;

  size_t media_playlists_outstanding_ = 0;
  size_t next_rendition_ = 0;
  size_t next_file_ = 0;
  size_t files_completed_ = 0;
  size_t total_files_ = 0;
  int64_t received_bytes_ = 0;
  bool finished_ = false;

  std::map<network::SimpleURLLoader*, std::unique_ptr<network::SimpleURLLoader>>
      in_flight_;

  base::WeakPtrFactory<PlaylistStreamDownloader> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_STREAM_DOWNLOADER_H_
