/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_DASH_PARSER_H_
#define BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_DASH_PARSER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "url/gurl.h"

namespace playlist {

// One DASH Representation, flattened into the files that make it up.
struct DashRepresentation {
  DashRepresentation();
  DashRepresentation(const DashRepresentation&);
  DashRepresentation& operator=(const DashRepresentation&);
  DashRepresentation(DashRepresentation&&) noexcept;
  DashRepresentation& operator=(DashRepresentation&&) noexcept;
  ~DashRepresentation();

  // The `EXT-X-MAP` equivalent. Empty when the representation has none.
  GURL initialization;
  std::vector<GURL> segments;
  // Parallel to `segments`.
  std::vector<base::TimeDelta> durations;

  uint64_t bandwidth = 0;
  std::string codecs;
  std::optional<int> width;
  std::optional<int> height;
};

// The one video and one audio representation worth saving. DASH keeps them in
// separate AdaptationSets, which is why saving a DASH stream needs the
// separate-rendition repackaging rather than a single file.
struct DashManifest {
  DashManifest();
  DashManifest(const DashManifest&) = delete;
  DashManifest& operator=(const DashManifest&) = delete;
  DashManifest(DashManifest&&) noexcept;
  DashManifest& operator=(DashManifest&&) noexcept;
  ~DashManifest();

  std::optional<DashRepresentation> video;
  std::optional<DashRepresentation> audio;
};

enum class DashParseError {
  kMalformed,
  // DRM protected: the bytes would never play back.
  kEncrypted,
  // Addressing we can't turn into a list of whole files, e.g. `SegmentBase`,
  // where every segment is a byte range of one remote file.
  kUnsupportedAddressing,
  kNoPlayableRepresentation,
};

using ParseDashManifestCallback =
    base::OnceCallback<void(base::expected<DashManifest, DashParseError>)>;

// Parses `xml` out of process, resolving relative URLs against `base_url`.
void ParseDashManifest(const std::string& xml,
                       const GURL& base_url,
                       ParseDashManifestCallback callback);

// Exposed for testing: turns the parsed XML tree into representations. `xml`
// parsing itself always happens in the sandbox.
base::expected<DashManifest, DashParseError> ParseDashManifestValueForTesting(
    const base::Value& root,
    const GURL& base_url);

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_DASH_PARSER_H_
