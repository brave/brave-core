/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/media_stream_classifier.h"

#include <string>

#include "base/containers/fixed_flat_set.h"
#include "base/files/file_path.h"
#include "base/strings/string_util.h"
#include "brave/components/playlist/content/browser/mime_util.h"
#include "net/base/mime_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace playlist {

namespace {

constexpr inline auto kHlsMimeTypes = base::MakeFixedFlatSet<std::string_view>({
    "application/x-mpegurl",
    "application/vnd.apple.mpegurl",
    "audio/x-mpegurl",
    "audio/mpegurl",
});

constexpr inline auto kDashMimeTypes =
    base::MakeFixedFlatSet<std::string_view>({"application/dash+xml"});

// MIME types servers hand out when they don't know or don't care. When we get
// one of these we fall back to the URL extension rather than giving up.
constexpr inline auto kUninformativeMimeTypes =
    base::MakeFixedFlatSet<std::string_view>(
        {"", "text/plain", "application/octet-stream", "binary/octet-stream",
         "application/xml", "text/xml"});

// Returns the lowercased extension of `url`'s path, without the dot, or an
// empty string. Query and ref are excluded by `GURL::path()`, so a
// "?f=x.m3u8" query can't masquerade as an extension.
base::FilePath::StringType GetUrlExtension(const GURL& url) {
  std::string_view path = url.path();
  const size_t slash = path.find_last_of('/');
  if (slash != std::string_view::npos) {
    path = path.substr(slash + 1);
  }

  const size_t dot = path.find_last_of('.');
  if (dot == std::string_view::npos) {
    return base::FilePath::StringType();
  }

  return base::FilePath::FromASCII(base::ToLowerASCII(path.substr(dot + 1)))
      .value();
}

bool IsMediaElementDestination(network::mojom::RequestDestination destination) {
  return destination == network::mojom::RequestDestination::kVideo ||
         destination == network::mojom::RequestDestination::kAudio;
}

// Detects if URL is likely an HLS/DASH manifest based on URL path pattern
// even without .m3u8/.mpd extension (e.g., YouTube's
// /api/manifest/hls_playlist/...)
}  // namespace

std::optional<MediaKind> ClassifyMediaResponse(
    const GURL& url,
    std::string_view mime_type,
    network::mojom::RequestDestination destination) {
  if (!url.SchemeIs(url::kHttpsScheme)) {
    // Playlist only ever stores https media. Bail before doing any work.
    return std::nullopt;
  }

  std::string mime = base::ToLowerASCII(mime_type);

  // Servers routinely serve media and manifests as octet-stream or text/plain.
  // When the MIME type tells us nothing, try to derive one from the URL
  // extension. Fallback to checking HLS/DASH MIME types directly (without URL
  // pattern matching).
  if (kUninformativeMimeTypes.contains(mime)) {
    const base::FilePath::StringType extension = GetUrlExtension(url);

    if (extension == FILE_PATH_LITERAL("mpd")) {
      return MediaKind::kDashManifest;
    }
    if (extension == FILE_PATH_LITERAL("m3u")) {
      return MediaKind::kHlsManifest;
    }
    if (auto from_extension =
            mime_util::GetMimeTypeForFileExtension(extension)) {
      mime = std::move(*from_extension);
    }
    // NOTE: No URL pattern matching - only use MIME type from headers
  }

  // Always check HLS/DASH MIME types directly from response headers
  if (kHlsMimeTypes.contains(mime)) {
    return MediaKind::kHlsManifest;
  }
  if (kDashMimeTypes.contains(mime)) {
    return MediaKind::kDashManifest;
  }

  const bool is_media_mime_type =
      net::MatchesMimeType("video/*", mime) ||
      net::MatchesMimeType("audio/*", mime) ||
      mime_util::GetFileExtensionForMimetype(mime).has_value();
  if (!is_media_mime_type) {
    return std::nullopt;
  }

  // A media element load is a whole file we can download directly, even though
  // Blink fetches it with Range requests.
  if (IsMediaElementDestination(destination)) {
    return MediaKind::kProgressive;
  }

  // Anything else fetching media bytes is a player driving MSE, so treat it as
  // a fragment. This deliberately gives up on the rare page that fetch()es a
  // whole file to make a blob URL out of it - calling that progressive would
  // mean offering to save individual six-second segments on every streaming
  // site, which is far worse.
  return MediaKind::kSegment;
}

bool IsHlsManifestUrl(const GURL& url) {
  const base::FilePath::StringType extension = GetUrlExtension(url);
  return extension == FILE_PATH_LITERAL("m3u8") ||
         extension == FILE_PATH_LITERAL("m3u");
}

bool IsStreamManifestUrl(const GURL& url) {
  if (IsHlsManifestUrl(url)) {
    return true;
  }
  const base::FilePath::StringType extension = GetUrlExtension(url);
  return extension == FILE_PATH_LITERAL("mpd");
}

}  // namespace playlist
