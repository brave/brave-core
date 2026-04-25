// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/yt_util.h"

#include <algorithm>
#include <functional>
#include <ios>
#include <optional>
#include <ostream>
#include <string>

#include "base/containers/checked_iterators.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/values.h"
#include "services/data_decoder/public/cpp/safe_xml_parser.h"

namespace ai_chat {

std::optional<std::string> ChooseCaptionTrackUrl(
    const base::ListValue& caption_tracks) {
  if (caption_tracks.empty()) {
    return std::nullopt;
  }
  const base::DictValue* track;
  // When only single track, use that
  if (caption_tracks.size() == 1) {
    track = caption_tracks.front().GetIfDict();
  } else {
    // When multiple tracks, favor english (due to ai_chat models), then first
    // english auto-generated track, then settle for anything.
    // TODO(petemill): Consider preferring user's language.
    auto iter =
        std::ranges::find_if(caption_tracks, [](const base::Value& track_raw) {
          const base::DictValue* language_track = track_raw.GetIfDict();
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
    if (iter == caption_tracks.end()) {
      iter = std::ranges::find_if(
          caption_tracks, [](const base::Value& track_raw) {
            const base::DictValue* language_track = track_raw.GetIfDict();
            auto* lang = language_track->FindString("languageCode");
            if (lang && *lang == "en") {
              return true;
            }
            return false;
          });
    }
    if (iter == caption_tracks.end()) {
      iter = caption_tracks.begin();
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

  return ChooseCaptionTrackUrl(*caption_tracks);
}

std::string ParseYoutubeTranscriptXml(const base::Value& root) {
  std::string transcript_text;

  if (data_decoder::IsXmlElementNamed(root, "timedtext")) {
    // Handle <timedtext> format ex.
    // <timedtext format="3">
    // <head>
    // <ws id="0"/>
    // <ws id="1" mh="2" ju="0" sd="3"/>
    // <wp id="0"/>
    // <wp id="1" ap="6" ah="20" av="100" rc="2" cc="40"/>
    // </head>
    // <body>
    // <w t="0" id="1" wp="1" ws="1"/>
    // <p t="160" d="4080" w="1">
    // <s ac="0">hi</s>
    // <s t="160" ac="0"> everyone</s>
    // <s t="1120" ac="0"> so</s>
    // <s t="1320" ac="0"> recently</s>
    // <s t="1720" ac="0"> I</s>
    // <s t="1840" ac="0"> gave</s>
    // <s t="1999" ac="0"> a</s>
    // </p>
    // <p t="2270" d="1970" w="1" a="1"> </p>
    // <p t="2280" d="4119" w="1">
    // <s ac="0">30-minute</s>
    // <s t="520" ac="0"> talk</s>
    // <s t="720" ac="0"> on</s>
    // <s t="879" ac="0"> large</s>
    // <s t="1159" ac="0"> language</s>
    // <s t="1480" ac="0"> models</s>
    // </p>
    // <p t="4230" d="2169" w="1" a="1"> </p>
    // ...
    // <p t="3585359" d="4321" w="1">
    // <s ac="0">keep</s>
    // <s t="161" ac="0"> track</s>
    // <s t="440" ac="0"> of</s>
    // <s t="1440" ac="0"> bye</s>
    // </p>
    // </body>
    // </timedtext>
    // or
    // <timedtext format="3">
    // <body>
    // <p t="13460" d="2175">Chris Anderson: This is such a strange thing.</p>
    // <p t="15659" d="3158">Your software, Linux, is in millions of
    // computers,</p>
    // <p t="18841" d="3547">it probably powers much of the Internet.</p>
    // <p t="22412" d="1763">And I think that there are, like,</p>
    // <p t="24199" d="3345">a billion and a half active Android devices out
    // there.</p>
    // <p t="27568" d="2601">Your software is in every single one of them.</p>
    // <p t="30808" d="1150">It's kind of amazing.</p>
    // <p t="31982" d="5035">You must have some amazing software headquarters
    // driving all this.</p>
    // <p t="37041" d="3306">That's what I thought -- and I was shocked when I
    // saw a picture of it.</p>
    // <p t="40371" d="1200">I mean, this is --</p>
    // <p t="41595" d="2250">this is the Linux world headquarters.</p>
    // ...
    // </body>
    // </timedtext>
    const base::ListValue* children = data_decoder::GetXmlElementChildren(root);
    if (!children) {
      return transcript_text;
    }
    for (const auto& child : *children) {
      if (!data_decoder::IsXmlElementNamed(child, "body")) {
        continue;
      }
      const base::ListValue* body_children =
          data_decoder::GetXmlElementChildren(child);
      if (!body_children) {
        continue;
      }
      for (const auto& p : *body_children) {
        if (!data_decoder::IsXmlElementNamed(p, "p")) {
          continue;
        }

        const base::ListValue* s_children =
            data_decoder::GetXmlElementChildren(p);
        bool all_s = s_children &&
                     std::ranges::all_of(*s_children, [](const base::Value& s) {
                       return data_decoder::IsXmlElementNamed(s, "s");
                     });

        if (all_s && s_children && !s_children->empty()) {
          if (!transcript_text.empty()) {
            transcript_text += " ";
          }
          // All children are <s>
          for (const auto& s : *s_children) {
            std::string s_text;
            if (data_decoder::GetXmlElementText(s, &s_text)) {
              transcript_text += s_text;
            }
          }
        } else {
          // Not all children are <s>, so treat as direct text
          std::string p_text;
          if (data_decoder::GetXmlElementText(p, &p_text)) {
            if (!transcript_text.empty()) {
              transcript_text += " ";
            }
            transcript_text += p_text;
          }
        }
      }
    }
  }

  return transcript_text;
}

}  // namespace ai_chat
