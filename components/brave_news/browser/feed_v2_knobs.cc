// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_v2_knobs.h"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece_forward.h"

namespace brave_news::knobs {

namespace {

std::string GetSwitch(const base::StringPiece& switch_name) {
  return base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
      switch_name);
}

int GetIntSwitchOrDefault(base::StringPiece name, int default_value) {
  auto v = GetSwitch(name);

  int result;
  if (!base::StringToInt(v, &result)) {
    return default_value;
  }

  return result;
}

double GetDoubleSwitchOrDefault(const base::StringPiece& name,
                                double default_value) {
  auto v = GetSwitch(name);
  double result;
  if (!base::StringToDouble(v, &result)) {
    return default_value;
  }
  return result;
}

}  // namespace

namespace switches {
const char kBraveNewsMinBlockCards[] = "brave-news-feed-min-block-cards";
const char kBraveNewsMaxBlockCards[] = "brave-news-feed-max-block-cards";

const char kBraveNewsPopRecencyHalfLife[] = "brave-news-pop-recency-half-life";
const char kBraveNewsPopRecencyFallback[] = "brave-news-pop-recency-fallback";

const char kBraveNewsInlineDiscoveryRatio[] =
    "brave-news-inline-discovery-ratio";
}  // namespace switches

int GetMinBlockCards() {
  constexpr int kDefault = 1;
  return GetIntSwitchOrDefault(switches::kBraveNewsMinBlockCards, kDefault);
}

int GetMaxBlockCards() {
  constexpr int kDefault = 5;
  return GetIntSwitchOrDefault(switches::kBraveNewsMaxBlockCards, kDefault);
}

double GetPopRecencyHalfLife() {
  constexpr double kDefault = 18;
  return GetDoubleSwitchOrDefault(switches::kBraveNewsPopRecencyHalfLife,
                                  kDefault);
}

double GetPopRecencyFallback() {
  constexpr double kDefault = 50;
  return GetDoubleSwitchOrDefault(switches::kBraveNewsPopRecencyFallback,
                                  kDefault);
}

double GetInlineDiscoveryRatio() {
  constexpr double kDefault = 0.25;
  return GetDoubleSwitchOrDefault(switches::kBraveNewsInlineDiscoveryRatio,
                                  kDefault);
}

}  // namespace brave_news::knobs
