/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_value_util.h"

#include <cstddef>
#include <string>
#include <utility>

#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/keyphrase/purchase_intent_keyphrase_parser.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr char kVersionKey[] = "version";

constexpr char kSegmentsKey[] = "segments";

constexpr char kSegmentKeyphrasesKey[] = "segment_keywords";

constexpr char kFunnelKeyphrasesKey[] = "funnel_keywords";

constexpr char kFunnelsKey[] = "funnel_sites";
constexpr char kFunnelSegmentsKey[] = "segments";
constexpr char kFunnelSitesKey[] = "sites";

constexpr int kDefaultFunnelSiteWeight = 1;

std::optional<SegmentList> ParseFunnelSegments(const SegmentList& segments,
                                               const base::Value::Dict& dict) {
  const auto* const funnel_segment_list = dict.FindList(kFunnelSegmentsKey);
  if (!funnel_segment_list) {
    return std::nullopt;
  }

  SegmentList funnel_segments;
  funnel_segments.reserve(funnel_segment_list->size());

  const size_t segments_size = segments.size();

  for (const auto& funnel_segment_value : *funnel_segment_list) {
    const std::optional<int> index = funnel_segment_value.GetIfInt();
    if (!index || index >= segments_size) {
      return std::nullopt;
    }

    funnel_segments.push_back(segments[*index]);
  }

  return funnel_segments;
}

}  // namespace

std::optional<int> ParseVersion(const base::Value::Dict& dict) {
  return dict.FindInt(kVersionKey);
}

std::optional<SegmentList> ParseSegments(const base::Value::Dict& dict) {
  const auto* const segment_list = dict.FindList(kSegmentsKey);
  if (!segment_list) {
    return std::nullopt;
  }

  SegmentList segments;
  segments.reserve(segment_list->size());

  for (const auto& segment_value : *segment_list) {
    const std::string* segment = segment_value.GetIfString();
    if (!segment || segment->empty()) {
      return std::nullopt;
    }

    segments.push_back(base::ToLowerASCII(*segment));
  }

  return segments;
}

std::optional<PurchaseIntentSegmentKeyphraseList> ParseSegmentKeyphrases(
    const SegmentList& segments,
    const base::Value::Dict& dict) {
  const auto* const segment_keyphrases_dict =
      dict.FindDict(kSegmentKeyphrasesKey);
  if (!segment_keyphrases_dict) {
    return std::nullopt;
  }

  PurchaseIntentSegmentKeyphraseList segment_keyphrases;
  segment_keyphrases.reserve(segment_keyphrases_dict->size());

  for (const auto [keyphrase, indexes_value] : *segment_keyphrases_dict) {
    if (!indexes_value.is_list()) {
      return std::nullopt;
    }

    PurchaseIntentSegmentKeyphraseInfo segment_keyphrase;
    segment_keyphrase.segments.reserve(indexes_value.GetList().size());
    segment_keyphrase.keywords = ParseKeyphrase(keyphrase);
    base::ranges::sort(segment_keyphrase.keywords);

    const size_t segments_size = segments.size();

    for (const auto& index_value : indexes_value.GetList()) {
      const std::optional<int> index = index_value.GetIfInt();
      if (!index || index >= segments_size) {
        return std::nullopt;
      }

      segment_keyphrase.segments.push_back(segments[*index]);
    }

    segment_keyphrases.push_back(std::move(segment_keyphrase));
  }

  return segment_keyphrases;
}

std::optional<PurchaseIntentFunnelKeyphraseList> ParseFunnelKeyphrases(
    const base::Value::Dict& dict) {
  const auto* const funnel_keyphrases_dict =
      dict.FindDict(kFunnelKeyphrasesKey);
  if (!funnel_keyphrases_dict) {
    return std::nullopt;
  }

  PurchaseIntentFunnelKeyphraseList funnel_keyphrases;
  funnel_keyphrases.reserve(funnel_keyphrases_dict->size());

  for (const auto [keyphrase, weight] : *funnel_keyphrases_dict) {
    if (!weight.is_int()) {
      return std::nullopt;
    }

    PurchaseIntentFunnelKeyphraseInfo funnel_keyphrase;

    funnel_keyphrase.keywords = ParseKeyphrase(keyphrase);
    base::ranges::sort(funnel_keyphrase.keywords);
    funnel_keyphrase.weight = weight.GetInt();

    funnel_keyphrases.push_back(std::move(funnel_keyphrase));
  }

  return funnel_keyphrases;
}

std::optional<PurchaseIntentFunnelSiteMap> ParseFunnelSites(
    const SegmentList& segments,
    const base::Value::Dict& dict) {
  const auto* const funnel_list = dict.FindList(kFunnelsKey);
  if (!funnel_list) {
    return std::nullopt;
  }

  PurchaseIntentFunnelSiteMap funnel_sites;

  for (const auto& funnel_value : *funnel_list) {
    const base::Value::Dict* funnel_dict = funnel_value.GetIfDict();
    if (!funnel_dict) {
      return std::nullopt;
    }

    const std::optional<SegmentList> funnel_segments =
        ParseFunnelSegments(segments, *funnel_dict);
    if (!funnel_segments) {
      return std::nullopt;
    }

    const auto* const funnel_site_list = funnel_dict->FindList(kFunnelSitesKey);
    if (!funnel_site_list) {
      return std::nullopt;
    }

    for (const auto& funnel_site_value : *funnel_site_list) {
      const std::string* funnel_site = funnel_site_value.GetIfString();
      if (!funnel_site) {
        return std::nullopt;
      }

      funnel_sites.insert({GURL(*funnel_site).GetWithEmptyPath().spec(),
                           PurchaseIntentFunnelInfo{*funnel_segments,
                                                    kDefaultFunnelSiteWeight}});
    }
  }

  return funnel_sites;
}

}  // namespace brave_ads
