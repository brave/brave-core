/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_network_media_detector.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/token.h"
#include "brave/components/playlist/content/browser/playlist_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "media/formats/hls/multivariant_playlist.h"
#include "media/formats/hls/playlist.h"
#include "media/formats/hls/rendition_group.h"
#include "media/formats/hls/variant_stream.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/origin.h"

namespace playlist {

namespace {

// How long to wait after finding media before handing it over. MediaSession
// metadata arrives once playback starts, which is after the first media
// response, so emitting immediately would produce untitled items.
constexpr base::TimeDelta kEmitDelay = base::Seconds(1);

// Manifests are text; anything this large is not one.
constexpr size_t kMaxManifestSize =
    network::SimpleURLLoader::kMaxBoundedStringDownloadSize;

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("playlist_network_media_detector",
                                             R"(
      semantics {
        sender: "Brave playlist media detector"
        description:
          "Fetches an HLS master playlist observed on the page to find the "
          "video-only and audio-only renditions it points to, so they can be "
          "recognized as parts of the same stream rather than separate "
          "playable items."
        trigger:
          "Playlist's V2 network detector observes an HLS master playlist "
          "response."
        data:
          "The master playlist"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
      })");
}

}  // namespace

PlaylistNetworkMediaDetector::PlaylistNetworkMediaDetector(
    content::WebContents* web_contents,
    MediaDetectedCallback on_media_detected)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<PlaylistNetworkMediaDetector>(*web_contents),
      media_session_observer_(std::make_unique<PlaylistMediaSessionObserver>(
          web_contents,
          base::BindRepeating(&PlaylistNetworkMediaDetector::OnMetadataChanged,
                              base::Unretained(this)))),
      network_observer_(PlaylistNetworkObserver::GetOrCreate(
          web_contents->GetBrowserContext())),
      on_media_detected_(std::move(on_media_detected)),
      url_loader_factory_(web_contents->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetURLLoaderFactoryForBrowserProcess()) {
  CHECK(on_media_detected_);
  network_observer_->AddObserver(this);
}

PlaylistNetworkMediaDetector::~PlaylistNetworkMediaDetector() {
  network_observer_->RemoveObserver(this);
}

void PlaylistNetworkMediaDetector::OnMediaResponseObserved(
    const content::GlobalRenderFrameHostToken& frame_token,
    const MediaResponseInfo& info) {
  // The network observer is per profile, so most responses belong to other
  // tabs.
  auto* render_frame_host =
      content::RenderFrameHost::FromFrameToken(frame_token);
  if (!render_frame_host || content::WebContents::FromRenderFrameHost(
                                render_frame_host) != web_contents()) {
    return;
  }

  // V2 deliberately leaves YouTube to the legacy detector and its
  // MediaSource-suppressed background resolution path.
  if (IsYoutubeLegacyPlaylistSite(web_contents()->GetLastCommittedURL())) {
    return;
  }

  // Bare MSE segments are recognized but not actionable on their own: they
  // only say that a stream exists, not where its manifest is.
  if (info.kind == MediaKind::kSegment) {
    return;
  }

  // A rendition of an HLS master playlist already seen on this page - not a
  // standalone item.
  if (suppressed_media_.contains(info.url)) {
    return;
  }

  if (emitted_media_.contains(info.url) ||
      std::ranges::contains(pending_media_, info.url)) {
    return;
  }

  pending_media_.push_back(info.url);
  emit_timer_.Start(FROM_HERE, kEmitDelay,
                    base::BindOnce(&PlaylistNetworkMediaDetector::Emit,
                                   base::Unretained(this)));

  if (info.kind == MediaKind::kHlsManifest) {
    MaybeDiscoverHlsRenditions(info.url);
  }
}

void PlaylistNetworkMediaDetector::OnMetadataChanged() {
  if (pending_media_.empty()) {
    return;
  }

  // Metadata just improved, so restart the wait - a title arriving 200ms from
  // now is worth more than emitting 200ms sooner.
  emit_timer_.Start(FROM_HERE, kEmitDelay,
                    base::BindOnce(&PlaylistNetworkMediaDetector::Emit,
                                   base::Unretained(this)));
}

void PlaylistNetworkMediaDetector::Emit() {
  if (pending_media_.empty()) {
    return;
  }

  const GURL page_url = web_contents()->GetLastCommittedURL();
  std::vector<mojom::PlaylistItemPtr> items;
  for (const auto& media_url : pending_media_) {
    items.push_back(MakeItem(page_url, media_url));
    emitted_media_.insert(media_url);
  }
  pending_media_.clear();

  on_media_detected_.Run(page_url, std::move(items));
}

mojom::PlaylistItemPtr PlaylistNetworkMediaDetector::MakeItem(
    const GURL& page_url,
    const GURL& media_url) const {
  const auto& metadata = media_session_observer_->metadata();

  auto item = mojom::PlaylistItem::New();
  item->id = base::Token::CreateRandom().ToString();
  item->page_source = page_url;
  item->page_redirected = page_url;
  item->media_source = media_url;
  item->media_path = media_url;
  item->is_blob_from_media_source = false;

  // Fall back to the tab title, which is what a user would call this anyway.
  item->name = base::UTF16ToUTF8(
      metadata.title.empty() ? web_contents()->GetTitle() : metadata.title);
  item->author = base::UTF16ToUTF8(metadata.artist);

  if (!metadata.duration.is_zero()) {
    item->duration = base::TimeDeltaToValue(metadata.duration).GetString();
  }
  if (!metadata.artwork.is_empty()) {
    item->thumbnail_source = metadata.artwork;
    item->thumbnail_path = metadata.artwork;
  }

  return item;
}

void PlaylistNetworkMediaDetector::SetURLLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  url_loader_factory_ = std::move(url_loader_factory);
}

void PlaylistNetworkMediaDetector::MaybeDiscoverHlsRenditions(
    const GURL& manifest_url) {
  if (!discovered_masters_.insert(manifest_url).second) {
    // Already fetched, or a fetch is already in flight, for this URL.
    return;
  }

  auto request = std::make_unique<network::ResourceRequest>();
  request->url = manifest_url;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;

  auto loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  loader->SetAllowHttpErrorResults(false);
  auto* loader_ptr = loader.get();
  in_flight_manifest_fetches_[loader_ptr] = std::move(loader);

  loader_ptr->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&PlaylistNetworkMediaDetector::OnHlsMasterPlaylistFetched,
                     weak_factory_.GetWeakPtr(), manifest_url, loader_ptr),
      kMaxManifestSize);
}

void PlaylistNetworkMediaDetector::OnHlsMasterPlaylistFetched(
    const GURL& manifest_url,
    network::SimpleURLLoader* loader,
    std::optional<std::string> body) {
  // `loader` is a parameter rather than a lambda capture on purpose: erasing
  // it here destroys the callback object that owns `body`, so nothing may be
  // read out of it afterwards.
  in_flight_manifest_fetches_.erase(loader);
  if (!body) {
    return;
  }

  auto identification = media::hls::Playlist::IdentifyPlaylist(*body);
  if (!identification.has_value()) {
    return;
  }
  const auto kind = std::move(identification).value();
  if (kind.kind != media::hls::Playlist::Kind::kMultivariantPlaylist) {
    // A plain media playlist has no renditions of its own to suppress.
    return;
  }

  auto parsed = media::hls::MultivariantPlaylist::Parse(
      *body, manifest_url, url::Origin::Create(manifest_url), kind.version);
  if (!parsed.has_value()) {
    return;
  }
  scoped_refptr<media::hls::MultivariantPlaylist> playlist =
      std::move(parsed).value();

  base::flat_set<GURL> renditions;
  for (const auto& variant : playlist->GetVariants()) {
    renditions.insert(variant.GetPrimaryRenditionUri());

    const auto& audio_group = variant.GetAudioRenditionGroup();
    if (audio_group.HasSharedTracks()) {
      if (auto track = audio_group.MostSimilar(std::nullopt)) {
        const auto* rendition = std::get<1>(*track).get();
        if (rendition && rendition->GetUri()) {
          renditions.insert(*rendition->GetUri());
        }
      }
    }
  }
  // The master itself is a legitimate item; only what it points to should be
  // suppressed.
  renditions.erase(manifest_url);

  if (renditions.empty()) {
    return;
  }

  suppressed_media_.insert(renditions.begin(), renditions.end());

  // Drop any rendition that snuck into the queue before the master finished
  // parsing.
  const size_t size_before = pending_media_.size();
  std::erase_if(pending_media_, [&renditions](const GURL& url) {
    return renditions.contains(url);
  });
  if (pending_media_.size() != size_before && !pending_media_.empty()) {
    // A rendition was just dropped from the queue; give MediaSession
    // metadata a fresh window before emitting what's left.
    emit_timer_.Start(FROM_HERE, kEmitDelay,
                      base::BindOnce(&PlaylistNetworkMediaDetector::Emit,
                                     base::Unretained(this)));
  }
}

void PlaylistNetworkMediaDetector::PrimaryPageChanged(content::Page& page) {
  emit_timer_.Stop();
  pending_media_.clear();
  emitted_media_.clear();
  suppressed_media_.clear();
  discovered_masters_.clear();
  in_flight_manifest_fetches_.clear();
  media_session_observer_->Reset();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistNetworkMediaDetector);

}  // namespace playlist
