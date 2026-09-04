/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_dash_parser.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/cpp/safe_xml_parser.h"

namespace playlist {

namespace {

// A hostile manifest shouldn't be able to make us enumerate an unbounded
// number of segments.
constexpr size_t kMaxSegmentsPerRepresentation = 20000;

std::optional<uint64_t> GetUint64Attribute(const base::Value& element,
                                           const std::string& name) {
  uint64_t value = 0;
  const std::string text = data_decoder::GetXmlElementAttribute(element, name);
  if (text.empty() || !base::StringToUint64(text, &value)) {
    return std::nullopt;
  }
  return value;
}

std::optional<int> GetIntAttribute(const base::Value& element,
                                   const std::string& name) {
  int value = 0;
  const std::string text = data_decoder::GetXmlElementAttribute(element, name);
  if (text.empty() || !base::StringToInt(text, &value)) {
    return std::nullopt;
  }
  return value;
}

// Parses the ISO 8601 durations DASH uses, e.g. "PT1H2M3.5S". Only the parts
// MPDs actually carry are handled.
std::optional<base::TimeDelta> ParseIsoDuration(const std::string& text) {
  if (text.size() < 3 || text[0] != 'P') {
    return std::nullopt;
  }

  const size_t time_start = text.find('T');
  if (time_start == std::string::npos) {
    return std::nullopt;
  }

  double total_seconds = 0;
  std::string number;
  for (size_t i = time_start + 1; i < text.size(); ++i) {
    const char c = text[i];
    if (base::IsAsciiDigit(c) || c == '.') {
      number.push_back(c);
      continue;
    }

    double value = 0;
    if (!base::StringToDouble(number, &value)) {
      return std::nullopt;
    }
    number.clear();

    switch (c) {
      case 'H':
        total_seconds += value * 3600;
        break;
      case 'M':
        total_seconds += value * 60;
        break;
      case 'S':
        total_seconds += value;
        break;
      default:
        return std::nullopt;
    }
  }

  return base::Seconds(total_seconds);
}

// Expands the `$Var$` placeholders DASH templates use. `$Number%05d$` style
// width specifiers are honoured, and `$$` is a literal dollar sign.
std::string ExpandTemplate(const std::string& tmpl,
                           const std::string& representation_id,
                           uint64_t bandwidth,
                           uint64_t number,
                           uint64_t time) {
  std::string out;
  for (size_t i = 0; i < tmpl.size();) {
    if (tmpl[i] != '$') {
      out.push_back(tmpl[i++]);
      continue;
    }

    const size_t end = tmpl.find('$', i + 1);
    if (end == std::string::npos) {
      // Unterminated placeholder: copy the rest through verbatim.
      out.append(tmpl.substr(i));
      break;
    }

    std::string token = tmpl.substr(i + 1, end - i - 1);
    i = end + 1;

    if (token.empty()) {
      out.push_back('$');
      continue;
    }

    // Split "Number%05d" into the identifier and its printf format.
    std::string format;
    if (const size_t percent = token.find('%'); percent != std::string::npos) {
      format = token.substr(percent);
      token = token.substr(0, percent);
    }

    const auto append_number = [&out, &format](uint64_t value) {
      if (format.empty()) {
        out.append(base::NumberToString(value));
        return;
      }
      // Only the "%0Nd" family appears in practice; anything else falls back
      // to a plain number rather than being handed to printf.
      if (format.size() < 2 || format.back() != 'd') {
        out.append(base::NumberToString(value));
        return;
      }
      int width = 0;
      base::StringToInt(format.substr(1, format.size() - 2), &width);
      out.append(
          absl::StrFormat("%0*llu", width, static_cast<uint64_t>(value)));
    };

    if (token == "RepresentationID") {
      out.append(representation_id);
    } else if (token == "Bandwidth") {
      append_number(bandwidth);
    } else if (token == "Number") {
      append_number(number);
    } else if (token == "Time") {
      append_number(time);
    } else {
      // Unknown placeholder: leave it alone rather than silently corrupting
      // the URL.
      out.append("$").append(token).append(format).append("$");
    }
  }

  return out;
}

// DASH lets any level carry a <BaseURL> that the next level resolves against.
GURL ResolveBaseUrl(const base::Value& element, const GURL& parent) {
  const base::Value* base_url_element =
      data_decoder::GetXmlElementChildWithTag(element, "BaseURL");
  if (!base_url_element) {
    return parent;
  }

  std::string text;
  if (!data_decoder::GetXmlElementText(*base_url_element, &text)) {
    return parent;
  }

  const GURL resolved = parent.Resolve(
      base::TrimWhitespaceASCII(text, base::TrimPositions::TRIM_ALL));
  return resolved.is_valid() ? resolved : parent;
}

bool HasContentProtection(const base::Value& element) {
  return data_decoder::GetXmlElementChildWithTag(
             element, "ContentProtection") != nullptr;
}

std::string GetContentType(const base::Value& adaptation_set,
                           const base::Value& representation) {
  std::string type =
      data_decoder::GetXmlElementAttribute(adaptation_set, "contentType");
  if (!type.empty()) {
    return type;
  }

  for (const auto* element : {&adaptation_set, &representation}) {
    const std::string mime_type =
        data_decoder::GetXmlElementAttribute(*element, "mimeType");
    if (mime_type.starts_with("video/")) {
      return "video";
    }
    if (mime_type.starts_with("audio/")) {
      return "audio";
    }
  }

  return std::string();
}

// Fills in `representation` from a <SegmentTemplate>, which is how live-profile
// MPDs (and most real ones) address segments.
bool BuildFromSegmentTemplate(const base::Value& segment_template,
                              const std::string& representation_id,
                              const GURL& base_url,
                              base::TimeDelta period_duration,
                              DashRepresentation& representation) {
  const uint64_t timescale =
      GetUint64Attribute(segment_template, "timescale").value_or(1u);
  if (timescale == 0) {
    return false;
  }

  const std::string initialization =
      data_decoder::GetXmlElementAttribute(segment_template, "initialization");
  if (!initialization.empty()) {
    representation.initialization = base_url.Resolve(
        ExpandTemplate(initialization, representation_id,
                       representation.bandwidth, /*number=*/0, /*time=*/0));
  }

  const std::string media =
      data_decoder::GetXmlElementAttribute(segment_template, "media");
  if (media.empty()) {
    return false;
  }

  uint64_t number =
      GetUint64Attribute(segment_template, "startNumber").value_or(1u);

  const base::Value* timeline = data_decoder::GetXmlElementChildWithTag(
      segment_template, "SegmentTimeline");
  if (timeline) {
    uint64_t time = 0;
    std::vector<const base::Value*> entries;
    data_decoder::GetAllXmlElementChildrenWithTag(*timeline, "S", &entries);
    for (const auto* entry : entries) {
      const auto duration_ticks = GetUint64Attribute(*entry, "d");
      if (!duration_ticks) {
        return false;
      }
      // `t` restarts the timeline; otherwise segments are contiguous.
      if (const auto start = GetUint64Attribute(*entry, "t")) {
        time = *start;
      }
      // A negative `r` means "repeat until the period ends", which we can't
      // resolve without trusting a duration we may not have.
      const int repeat = GetIntAttribute(*entry, "r").value_or(0);
      if (repeat < 0) {
        return false;
      }

      for (int i = 0; i <= repeat; ++i) {
        if (representation.segments.size() >= kMaxSegmentsPerRepresentation) {
          return false;
        }
        representation.segments.push_back(base_url.Resolve(ExpandTemplate(
            media, representation_id, representation.bandwidth, number, time)));
        representation.durations.push_back(
            base::Seconds(static_cast<double>(*duration_ticks) /
                          static_cast<double>(timescale)));
        time += *duration_ticks;
        ++number;
      }
    }

    return !representation.segments.empty();
  }

  // No timeline: every segment has the same duration, and the count comes from
  // how long the period lasts.
  const auto duration_ticks = GetUint64Attribute(segment_template, "duration");
  if (!duration_ticks || *duration_ticks == 0 || period_duration.is_zero()) {
    return false;
  }

  const base::TimeDelta segment_duration = base::Seconds(
      static_cast<double>(*duration_ticks) / static_cast<double>(timescale));
  const double count =
      std::ceil(period_duration.InSecondsF() / segment_duration.InSecondsF());
  if (count <= 0 || count > kMaxSegmentsPerRepresentation) {
    return false;
  }

  for (uint64_t i = 0; i < static_cast<uint64_t>(count); ++i) {
    representation.segments.push_back(base_url.Resolve(
        ExpandTemplate(media, representation_id, representation.bandwidth,
                       number + i, i * *duration_ticks)));
    representation.durations.push_back(segment_duration);
  }

  return true;
}

// Fills in `representation` from a <SegmentList>, the explicit form.
bool BuildFromSegmentList(const base::Value& segment_list,
                          const GURL& base_url,
                          DashRepresentation& representation) {
  const uint64_t timescale =
      GetUint64Attribute(segment_list, "timescale").value_or(1u);
  const auto duration_ticks = GetUint64Attribute(segment_list, "duration");
  if (timescale == 0 || !duration_ticks) {
    return false;
  }

  if (const base::Value* initialization =
          data_decoder::GetXmlElementChildWithTag(segment_list,
                                                  "Initialization")) {
    const std::string source_url =
        data_decoder::GetXmlElementAttribute(*initialization, "sourceURL");
    if (source_url.empty()) {
      // An <Initialization> with only a byte range points into a single file.
      return false;
    }
    representation.initialization = base_url.Resolve(source_url);
  }

  const base::TimeDelta segment_duration = base::Seconds(
      static_cast<double>(*duration_ticks) / static_cast<double>(timescale));

  std::vector<const base::Value*> entries;
  data_decoder::GetAllXmlElementChildrenWithTag(segment_list, "SegmentURL",
                                                &entries);
  for (const auto* entry : entries) {
    const std::string media =
        data_decoder::GetXmlElementAttribute(*entry, "media");
    if (media.empty() ||
        !data_decoder::GetXmlElementAttribute(*entry, "mediaRange").empty()) {
      return false;
    }
    if (representation.segments.size() >= kMaxSegmentsPerRepresentation) {
      return false;
    }
    representation.segments.push_back(base_url.Resolve(media));
    representation.durations.push_back(segment_duration);
  }

  return !representation.segments.empty();
}

}  // namespace

DashRepresentation::DashRepresentation() = default;
DashRepresentation::DashRepresentation(const DashRepresentation&) = default;
DashRepresentation& DashRepresentation::operator=(const DashRepresentation&) =
    default;
DashRepresentation::DashRepresentation(DashRepresentation&&) noexcept = default;
DashRepresentation& DashRepresentation::operator=(
    DashRepresentation&&) noexcept = default;
DashRepresentation::~DashRepresentation() = default;

DashManifest::DashManifest() = default;
DashManifest::DashManifest(DashManifest&&) noexcept = default;
DashManifest& DashManifest::operator=(DashManifest&&) noexcept = default;
DashManifest::~DashManifest() = default;

base::expected<DashManifest, DashParseError>
ParseDashManifestValueForTesting(  // IN-TEST
    const base::Value& root,
    const GURL& base_url) {
  if (!data_decoder::IsXmlElementNamed(root, "MPD")) {
    return base::unexpected(DashParseError::kMalformed);
  }

  const base::Value* period =
      data_decoder::GetXmlElementChildWithTag(root, "Period");
  if (!period) {
    return base::unexpected(DashParseError::kMalformed);
  }

  base::TimeDelta period_duration;
  for (const auto* element : {period, &root}) {
    const std::string attribute =
        element == period
            ? data_decoder::GetXmlElementAttribute(*element, "duration")
            : data_decoder::GetXmlElementAttribute(*element,
                                                   "mediaPresentationDuration");
    if (auto parsed = ParseIsoDuration(attribute)) {
      period_duration = *parsed;
      break;
    }
  }

  const GURL period_base_url =
      ResolveBaseUrl(*period, ResolveBaseUrl(root, base_url));

  std::vector<const base::Value*> adaptation_sets;
  data_decoder::GetAllXmlElementChildrenWithTag(*period, "AdaptationSet",
                                                &adaptation_sets);

  DashManifest manifest;
  for (const auto* adaptation_set : adaptation_sets) {
    if (HasContentProtection(*adaptation_set)) {
      return base::unexpected(DashParseError::kEncrypted);
    }

    const GURL adaptation_base_url =
        ResolveBaseUrl(*adaptation_set, period_base_url);

    std::vector<const base::Value*> representations;
    data_decoder::GetAllXmlElementChildrenWithTag(
        *adaptation_set, "Representation", &representations);

    // Keep the best rendition: the user asked to save this, not to stream it.
    const base::Value* best = nullptr;
    uint64_t best_bandwidth = 0;
    for (const auto* candidate : representations) {
      if (HasContentProtection(*candidate)) {
        return base::unexpected(DashParseError::kEncrypted);
      }
      const uint64_t bandwidth =
          GetUint64Attribute(*candidate, "bandwidth").value_or(0u);
      if (!best || bandwidth > best_bandwidth) {
        best = candidate;
        best_bandwidth = bandwidth;
      }
    }

    if (!best) {
      continue;
    }

    const std::string content_type = GetContentType(*adaptation_set, *best);
    const bool is_video = content_type == "video";
    if (!is_video && content_type != "audio") {
      // Subtitles and anything else we don't save.
      continue;
    }
    if (is_video ? manifest.video.has_value() : manifest.audio.has_value()) {
      continue;
    }

    DashRepresentation representation;
    representation.bandwidth = best_bandwidth;
    representation.codecs =
        data_decoder::GetXmlElementAttribute(*best, "codecs");
    if (representation.codecs.empty()) {
      representation.codecs =
          data_decoder::GetXmlElementAttribute(*adaptation_set, "codecs");
    }
    if (is_video) {
      representation.width = GetIntAttribute(*best, "width");
      representation.height = GetIntAttribute(*best, "height");
    }

    const GURL representation_base_url =
        ResolveBaseUrl(*best, adaptation_base_url);
    const std::string representation_id =
        data_decoder::GetXmlElementAttribute(*best, "id");

    // A <SegmentTemplate> may sit on the AdaptationSet rather than the
    // Representation.
    const base::Value* segment_template =
        data_decoder::GetXmlElementChildWithTag(*best, "SegmentTemplate");
    if (!segment_template) {
      segment_template = data_decoder::GetXmlElementChildWithTag(
          *adaptation_set, "SegmentTemplate");
    }
    const base::Value* segment_list =
        data_decoder::GetXmlElementChildWithTag(*best, "SegmentList");

    bool built = false;
    if (segment_template) {
      built = BuildFromSegmentTemplate(*segment_template, representation_id,
                                       representation_base_url, period_duration,
                                       representation);
    } else if (segment_list) {
      built = BuildFromSegmentList(*segment_list, representation_base_url,
                                   representation);
    } else {
      // <SegmentBase>, or a bare <BaseURL> file: every "segment" is a byte
      // range of one remote file, which we don't store.
      return base::unexpected(DashParseError::kUnsupportedAddressing);
    }

    if (!built) {
      return base::unexpected(DashParseError::kUnsupportedAddressing);
    }

    (is_video ? manifest.video : manifest.audio) = std::move(representation);
  }

  if (!manifest.video && !manifest.audio) {
    return base::unexpected(DashParseError::kNoPlayableRepresentation);
  }

  return manifest;
}

void ParseDashManifest(const std::string& xml,
                       const GURL& base_url,
                       ParseDashManifestCallback callback) {
  data_decoder::DataDecoder::ParseXmlIsolated(
      xml, data_decoder::mojom::XmlParser::WhitespaceBehavior::kIgnore,
      base::BindOnce(
          [](GURL base_url, ParseDashManifestCallback callback,
             data_decoder::DataDecoder::ValueOrError result) {
            if (!result.has_value()) {
              std::move(callback).Run(
                  base::unexpected(DashParseError::kMalformed));
              return;
            }
            std::move(callback).Run(
                ParseDashManifestValueForTesting(  // IN-TEST
                    *result, base_url));
          },
          base_url, std::move(callback)));
}

}  // namespace playlist
