/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_stream_downloader.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/thread_pool.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "media/formats/hls/media_playlist.h"
#include "media/formats/hls/media_segment.h"
#include "media/formats/hls/multivariant_playlist.h"
#include "media/formats/hls/playlist.h"
#include "media/formats/hls/tags.h"
#include "media/formats/hls/variant_stream.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/origin.h"

namespace playlist {

namespace {

// Manifests are text; anything this large is not one. `DownloadToString`
// refuses anything above `kMaxBoundedStringDownloadSize`.
constexpr size_t kMaxManifestSize =
    network::SimpleURLLoader::kMaxBoundedStringDownloadSize;

// Segments are fetched several at a time. Streams routinely have hundreds of
// small segments, and one round trip each would dominate the download.
constexpr size_t kMaxConcurrentSegmentDownloads = 4;

constexpr char kPrimaryPlaylistFileName[] = "video.m3u8";
constexpr char kAudioPlaylistFileName[] = "audio.m3u8";
constexpr char kMultivariantFileName[] = "master.m3u8";
// Written when the stream needs no multivariant wrapper, so the caller always
// has one entry point.
constexpr char kSingleRenditionFileName[] = "media.m3u8";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("playlist_stream_downloader", R"(
      semantics {
        sender: "Brave playlist HLS downloader"
        description:
          "Downloads the manifest and media segments of a stream the user "
          "asked to save to their playlist, so it can be played offline."
        trigger:
          "User-initiated when saving a stream to a playlist"
        data:
          "The stream's manifest and media segments"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
      })");
}

// Segments are named by position rather than by their remote URL: remote names
// collide across renditions, can be absent entirely, and are attacker
// controlled.
std::string MakeLocalFileName(const std::string& prefix,
                              size_t index,
                              const GURL& url) {
  std::string extension;
  const std::string_view path = url.path();
  if (const size_t slash = path.find_last_of('/');
      slash != std::string_view::npos) {
    const std::string_view name = path.substr(slash + 1);
    if (const size_t dot = name.find_last_of('.');
        dot != std::string_view::npos) {
      const std::string_view candidate = name.substr(dot + 1);
      // Only take an extension that plainly looks like one.
      if (!candidate.empty() && candidate.size() <= 4 &&
          std::ranges::all_of(
              candidate, [](char c) { return base::IsAsciiAlphaNumeric(c); })) {
        extension = base::ToLowerASCII(candidate);
      }
    }
  }

  if (extension.empty()) {
    extension = "bin";
  }

  return base::StrCat({prefix, absl::StrFormat("_%05zu.", index), extension});
}

}  // namespace

// static
std::string_view PlaylistStreamDownloader::ErrorToString(Error error) {
  switch (error) {
    case Error::kNetwork:
      return "network error";
    case Error::kParse:
      return "could not parse the manifest";
    case Error::kEncrypted:
      return "stream is encrypted";
    case Error::kNoPlayableRendition:
      return "no playable rendition";
    case Error::kFileSystem:
      return "could not write to disk";
  }
}

PlaylistStreamDownloader::PlaylistStreamDownloader(
    content::BrowserContext* context)
    : url_loader_factory_(context->GetDefaultStoragePartition()
                              ->GetURLLoaderFactoryForBrowserProcess()) {}

PlaylistStreamDownloader::~PlaylistStreamDownloader() = default;

void PlaylistStreamDownloader::Start(const GURL& manifest_url,
                                     const base::FilePath& destination_dir,
                                     ProgressCallback on_progress,
                                     ResultCallback on_result) {
  CHECK(on_result);
  manifest_url_ = manifest_url;
  destination_dir_ = destination_dir;
  on_progress_ = std::move(on_progress);
  on_result_ = std::move(on_result);

  FetchManifest(manifest_url_,
                base::BindOnce(&PlaylistStreamDownloader::OnRootManifestFetched,
                               weak_factory_.GetWeakPtr()));
}

void PlaylistStreamDownloader::FetchManifest(
    const GURL& url,
    base::OnceCallback<void(std::optional<std::string>)> callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;

  auto loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  loader->SetAllowHttpErrorResults(false);
  auto* loader_ptr = loader.get();
  in_flight_[loader_ptr] = std::move(loader);

  loader_ptr->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&PlaylistStreamDownloader::OnManifestFetched,
                     weak_factory_.GetWeakPtr(), loader_ptr,
                     std::move(callback)),
      kMaxManifestSize);
}

void PlaylistStreamDownloader::OnManifestFetched(
    network::SimpleURLLoader* loader,
    base::OnceCallback<void(std::optional<std::string>)> callback,
    std::optional<std::string> body) {
  // `callback` and `body` are parameters rather than lambda captures on
  // purpose: dropping the loader here destroys the callback object that owns
  // the bound state, so nothing may be read out of it afterwards.
  in_flight_.erase(loader);
  std::move(callback).Run(std::move(body));
}

void PlaylistStreamDownloader::OnRootManifestFetched(
    std::optional<std::string> body) {
  if (!body) {
    Fail(Error::kNetwork);
    return;
  }

  // Sniff rather than trust the URL: an MPD is XML, an HLS playlist is not.
  if (base::TrimWhitespaceASCII(*body, base::TRIM_LEADING).starts_with("<")) {
    ParseDashManifest(
        *body, manifest_url_,
        base::BindOnce(&PlaylistStreamDownloader::OnDashManifestParsed,
                       weak_factory_.GetWeakPtr()));
    return;
  }

  auto identification = media::hls::Playlist::IdentifyPlaylist(*body);
  if (!identification.has_value()) {
    Fail(Error::kParse);
    return;
  }

  const auto kind = std::move(identification).value();
  const url::Origin origin = url::Origin::Create(manifest_url_);

  if (kind.kind == media::hls::Playlist::Kind::kMediaPlaylist) {
    renditions_.push_back(
        RenditionJob{.playlist_file_name = kSingleRenditionFileName});
    if (!BuildRenditionJob(0, *body)) {
      return;
    }
    MaybeStartSegmentDownloads();
    return;
  }

  auto parsed = media::hls::MultivariantPlaylist::Parse(*body, manifest_url_,
                                                        origin, kind.version);
  if (!parsed.has_value()) {
    Fail(Error::kParse);
    return;
  }

  scoped_refptr<media::hls::MultivariantPlaylist> playlist =
      std::move(parsed).value();

  // Highest bandwidth wins: the user asked to keep this, so keep the best
  // rendition rather than whatever the player happened to be streaming.
  const media::hls::VariantStream* best = nullptr;
  for (const auto& variant : playlist->GetVariants()) {
    if (!best || variant.GetBandwidth() > best->GetBandwidth()) {
      best = &variant;
    }
  }

  if (!best) {
    Fail(Error::kNoPlayableRendition);
    return;
  }

  const auto& audio_group = best->GetAudioRenditionGroup();
  std::optional<GURL> audio_url;
  if (audio_group.HasSharedTracks()) {
    if (auto track = audio_group.MostSimilar(std::nullopt)) {
      const auto& rendition = std::get<1>(*track);
      if (rendition && rendition->GetUri() &&
          *rendition->GetUri() != best->GetPrimaryRenditionUri()) {
        audio_url = *rendition->GetUri();
      }
    }
  }

  renditions_.push_back(
      RenditionJob{.playlist_file_name = audio_url ? kPrimaryPlaylistFileName
                                                   : kSingleRenditionFileName});

  if (audio_url) {
    renditions_.push_back(
        RenditionJob{.playlist_file_name = kAudioPlaylistFileName});

    HlsMultivariant multivariant;
    multivariant.video_playlist_file_name = kPrimaryPlaylistFileName;
    multivariant.audio_playlist_file_name = kAudioPlaylistFileName;
    multivariant.bandwidth = best->GetBandwidth();
    if (const auto& codecs = best->GetCodecs()) {
      multivariant.codecs = base::JoinString(*codecs, ",");
    }
    if (const auto resolution = best->GetResolution()) {
      multivariant.width = static_cast<int>(resolution->width);
      multivariant.height = static_cast<int>(resolution->height);
    }
    multivariant_ = std::move(multivariant);
  }

  media_playlists_outstanding_ = renditions_.size();
  FetchManifest(
      best->GetPrimaryRenditionUri(),
      base::BindOnce(&PlaylistStreamDownloader::OnMediaPlaylistFetched,
                     weak_factory_.GetWeakPtr(), 0u));
  if (audio_url) {
    FetchManifest(
        *audio_url,
        base::BindOnce(&PlaylistStreamDownloader::OnMediaPlaylistFetched,
                       weak_factory_.GetWeakPtr(), 1u));
  }
}

void PlaylistStreamDownloader::OnDashManifestParsed(
    base::expected<DashManifest, DashParseError> parsed) {
  if (finished_) {
    return;
  }

  if (!parsed.has_value()) {
    switch (parsed.error()) {
      case DashParseError::kEncrypted:
        Fail(Error::kEncrypted);
        return;
      case DashParseError::kMalformed:
        Fail(Error::kParse);
        return;
      case DashParseError::kUnsupportedAddressing:
      case DashParseError::kNoPlayableRepresentation:
        Fail(Error::kNoPlayableRendition);
        return;
    }
  }

  // DASH keeps audio and video in separate adaptation sets. With both present
  // they become two local renditions bound by a multivariant manifest, which
  // is what avoids having to mux them.
  const bool separated = parsed->video && parsed->audio;

  const auto add_rendition = [this](const DashRepresentation& source,
                                    const std::string& prefix,
                                    const std::string& playlist_file_name) {
    RenditionJob job;
    job.playlist_file_name = playlist_file_name;

    if (source.initialization.is_valid()) {
      const std::string file_name = base::StrCat({prefix, "_init.mp4"});
      job.manifest.init_file_name = file_name;
      job.files.push_back(PendingFile{source.initialization, file_name});
    }

    for (size_t i = 0; i < source.segments.size(); ++i) {
      const std::string file_name =
          MakeLocalFileName(prefix, i, source.segments[i]);
      job.manifest.segments.push_back(
          HlsSegmentEntry{file_name, source.durations[i]});
      job.files.push_back(PendingFile{source.segments[i], file_name});
    }

    renditions_.push_back(std::move(job));
  };

  // Index 0 must be the video rendition, since that's what the rest of the
  // class treats as primary.
  if (parsed->video) {
    add_rendition(
        *parsed->video, "v",
        separated ? kPrimaryPlaylistFileName : kSingleRenditionFileName);
  }
  if (parsed->audio) {
    add_rendition(
        *parsed->audio, "a",
        separated ? kAudioPlaylistFileName : kSingleRenditionFileName);
  }

  if (separated) {
    HlsMultivariant multivariant;
    multivariant.video_playlist_file_name = kPrimaryPlaylistFileName;
    multivariant.audio_playlist_file_name = kAudioPlaylistFileName;
    multivariant.bandwidth =
        parsed->video->bandwidth + parsed->audio->bandwidth;
    // The variant has to advertise both codecs, since it plays both.
    multivariant.codecs = parsed->video->codecs;
    if (!parsed->audio->codecs.empty()) {
      multivariant.codecs =
          multivariant.codecs.empty()
              ? parsed->audio->codecs
              : base::StrCat({multivariant.codecs, ",", parsed->audio->codecs});
    }
    multivariant.width = parsed->video->width;
    multivariant.height = parsed->video->height;
    multivariant_ = std::move(multivariant);
  }

  MaybeStartSegmentDownloads();
}

void PlaylistStreamDownloader::OnMediaPlaylistFetched(
    size_t rendition_index,
    std::optional<std::string> body) {
  if (finished_) {
    return;
  }

  if (!body) {
    Fail(Error::kNetwork);
    return;
  }

  if (!BuildRenditionJob(rendition_index, *body)) {
    return;
  }

  CHECK_GT(media_playlists_outstanding_, 0u);
  if (--media_playlists_outstanding_ == 0) {
    MaybeStartSegmentDownloads();
  }
}

bool PlaylistStreamDownloader::BuildRenditionJob(size_t rendition_index,
                                                 std::string_view body) {
  auto identification = media::hls::Playlist::IdentifyPlaylist(body);
  if (!identification.has_value()) {
    Fail(Error::kParse);
    return false;
  }

  auto parsed = media::hls::MediaPlaylist::Parse(
      body, manifest_url_, url::Origin::Create(manifest_url_),
      std::move(identification).value().version,
      /*parent_playlist=*/nullptr);
  if (!parsed.has_value()) {
    Fail(Error::kParse);
    return false;
  }

  scoped_refptr<media::hls::MediaPlaylist> playlist = std::move(parsed).value();
  if (playlist->GetSegments().empty()) {
    Fail(Error::kNoPlayableRendition);
    return false;
  }

  // A live playlist has no end, so there's nothing complete to save.
  if (!playlist->IsEndList()) {
    Fail(Error::kNoPlayableRendition);
    return false;
  }

  RenditionJob& job = renditions_[rendition_index];
  const std::string prefix = rendition_index == 0 ? "v" : "a";

  for (const auto& segment : playlist->GetSegments()) {
    // We store bytes verbatim and can't decrypt them later, so refuse rather
    // than save something that will never play.
    if (auto encryption = segment->GetEncryptionData();
        encryption &&
        encryption->GetMethod() != media::hls::XKeyTagMethod::kNone) {
      Fail(Error::kEncrypted);
      return false;
    }

    // A byte range means several segments share one remote file. Saving them
    // verbatim would need the range preserved in the local manifest, which we
    // don't write yet.
    if (segment->GetByteRange()) {
      Fail(Error::kNoPlayableRendition);
      return false;
    }

    if (auto init = segment->GetInitializationSegment();
        init && !job.manifest.init_file_name) {
      const std::string file_name = base::StrCat({prefix, "_init.", "mp4"});
      job.manifest.init_file_name = file_name;
      job.files.push_back(PendingFile{init->GetUri(), file_name});
    }

    const std::string file_name = MakeLocalFileName(
        prefix, job.manifest.segments.size(), segment->GetUri());
    job.manifest.segments.push_back(
        HlsSegmentEntry{file_name, segment->GetDuration()});
    job.files.push_back(PendingFile{segment->GetUri(), file_name});
  }

  return true;
}

void PlaylistStreamDownloader::MaybeStartSegmentDownloads() {
  if (finished_) {
    return;
  }

  total_files_ = 0;
  for (const auto& rendition : renditions_) {
    total_files_ += rendition.files.size();
  }

  if (total_files_ == 0) {
    Fail(Error::kNoPlayableRendition);
    return;
  }

  for (size_t i = 0; i < kMaxConcurrentSegmentDownloads; ++i) {
    StartNextSegmentDownload();
  }
}

void PlaylistStreamDownloader::StartNextSegmentDownload() {
  if (finished_) {
    return;
  }

  while (next_rendition_ < renditions_.size() &&
         next_file_ >= renditions_[next_rendition_].files.size()) {
    ++next_rendition_;
    next_file_ = 0;
  }

  if (next_rendition_ >= renditions_.size()) {
    return;
  }

  const size_t rendition_index = next_rendition_;
  const size_t file_index = next_file_++;
  const PendingFile& file = renditions_[rendition_index].files[file_index];

  auto request = std::make_unique<network::ResourceRequest>();
  request->url = file.url;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;

  auto loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  loader->SetAllowHttpErrorResults(false);
  const unsigned int kRetriesOnNetworkChange = 1;
  loader->SetRetryOptions(
      kRetriesOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);

  auto* loader_ptr = loader.get();
  in_flight_[loader_ptr] = std::move(loader);

  loader_ptr->DownloadToFile(
      url_loader_factory_.get(),
      base::BindOnce(&PlaylistStreamDownloader::OnSegmentDownloaded,
                     weak_factory_.GetWeakPtr(), rendition_index, file_index,
                     loader_ptr),
      destination_dir_.AppendASCII(file.file_name));
}

void PlaylistStreamDownloader::OnSegmentDownloaded(
    size_t rendition_index,
    size_t file_index,
    network::SimpleURLLoader* loader,
    base::FilePath path) {
  in_flight_.erase(loader);

  if (finished_) {
    return;
  }

  if (path.empty()) {
    Fail(Error::kNetwork);
    return;
  }

  ++files_completed_;

  // `DownloadToFile` doesn't report sizes, and stat-ing every segment on the
  // UI thread isn't worth it; the manifest bytes are negligible either way.
  if (on_progress_) {
    const int percent = static_cast<int>(files_completed_ * 100 / total_files_);
    on_progress_.Run(received_bytes_, percent);
  }

  if (files_completed_ == total_files_) {
    WriteManifests();
    return;
  }

  StartNextSegmentDownload();
}

void PlaylistStreamDownloader::WriteManifests() {
  std::vector<std::pair<base::FilePath, std::string>> to_write;
  for (const auto& rendition : renditions_) {
    to_write.emplace_back(
        destination_dir_.AppendASCII(rendition.playlist_file_name),
        WriteHlsMediaPlaylist(rendition.manifest));
  }

  std::string entry_point = renditions_[0].playlist_file_name;
  if (multivariant_) {
    to_write.emplace_back(destination_dir_.AppendASCII(kMultivariantFileName),
                          WriteHlsMultivariantPlaylist(*multivariant_));
    entry_point = kMultivariantFileName;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(
          [](std::vector<std::pair<base::FilePath, std::string>> files) {
            for (const auto& [path, contents] : files) {
              if (!base::WriteFile(path, contents)) {
                return false;
              }
            }
            return true;
          },
          std::move(to_write)),
      base::BindOnce(&PlaylistStreamDownloader::OnManifestsWritten,
                     weak_factory_.GetWeakPtr()));

  pending_entry_point_ = std::move(entry_point);
}

void PlaylistStreamDownloader::OnManifestsWritten(bool success) {
  if (!success) {
    Fail(Error::kFileSystem);
    return;
  }

  Succeed(pending_entry_point_);
}

void PlaylistStreamDownloader::Fail(Error error) {
  if (finished_) {
    return;
  }

  finished_ = true;
  in_flight_.clear();
  VLOG(1) << __func__ << ": " << ErrorToString(error) << " for "
          << manifest_url_.spec();
  std::move(on_result_).Run(base::unexpected(error));
}

void PlaylistStreamDownloader::Succeed(const std::string& manifest_file_name) {
  if (finished_) {
    return;
  }

  finished_ = true;
  in_flight_.clear();
  std::move(on_result_)
      .Run(Result{.manifest_file_name = manifest_file_name,
                  .received_bytes = received_bytes_});
}

}  // namespace playlist
