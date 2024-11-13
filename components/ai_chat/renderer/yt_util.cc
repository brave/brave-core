// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/renderer/yt_util.h"

#include <functional>
#include <ios>
#include <optional>
#include <ostream>
#include <string>

#include "base/containers/checked_iterators.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/values.h"

namespace ai_chat {

std::optional<std::string> ChooseCaptionTrackUrl(
    const base::Value::List* caption_tracks) {
  if (!caption_tracks || caption_tracks->empty()) {
    return std::nullopt;
  }
  if (caption_tracks->empty()) {
    return std::nullopt;
  }
  const base::Value::Dict* track;
  // When only single track, use that
  if (caption_tracks->size() == 1) {
    track = caption_tracks->front().GetIfDict();
  } else {
    // When multiple tracks, favor english (due to ai_chat models), then first
    // english auto-generated track, then settle for anything.
    // TODO(petemill): Consider preferring user's language.
    auto iter = base::ranges::find_if(
        *caption_tracks, [](const base::Value& track_raw) {
          const base::Value::Dict* language_track = track_raw.GetIfDict();
          auto* kind = language_track->FindString("kind");
          if (kind && *kind == "asr") {
            return false;
          }
          auto* lang = language_track->FindString("languageCode");
          if (lang && *lang == "en") {
            return true;
          }
          return false;
        });
    if (iter == caption_tracks->end()) {
      iter = base::ranges::find_if(
          *caption_tracks, [](const base::Value& track_raw) {
            const base::Value::Dict* language_track = track_raw.GetIfDict();
            auto* lang = language_track->FindString("languageCode");
            if (lang && *lang == "en") {
              return true;
            }
            return false;
          });
    }
    if (iter == caption_tracks->end()) {
      iter = caption_tracks->begin();
    }
    track = iter->GetIfDict();
  }
  if (!track) {
    return std::nullopt;
  }
  const std::string* caption_url_raw = track->FindString("baseUrl");

  if (!caption_url_raw) {
    return std::nullopt;
  }
  return *caption_url_raw;
}

std::optional<std::string> ParseAndChooseCaptionTrackUrl(
    std::string_view body) {
  if (!body.size()) {
    return std::nullopt;
  }

  auto result_value =
      base::JSONReader::ReadAndReturnValueWithError(body, base::JSON_PARSE_RFC);

  if (!result_value.has_value() || result_value->is_string()) {
    DVLOG(1) << __func__ << ": parsing error: " << result_value.ToString();
    return std::nullopt;
  } else if (!result_value->is_dict()) {
    DVLOG(1) << __func__ << ": parsing error: not a dict";
    return std::nullopt;
  }

  auto* caption_tracks = result_value->GetDict().FindListByDottedPath(
      "captions.playerCaptionsTracklistRenderer.captionTracks");
  if (!caption_tracks) {
    DVLOG(1) << __func__ << ": no caption tracks found";
    return std::nullopt;
  }

  return ChooseCaptionTrackUrl(caption_tracks);
}

}  // namespace ai_chat
