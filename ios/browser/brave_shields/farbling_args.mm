// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/farbling_args.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "base/token.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "third_party/abseil-cpp/absl/random/random.h"

namespace brave_shields {

namespace {

// Seeded pseudo-random generator used to deterministically derive farbling
// values for a given origin. This is the same generator that the rest of
// Brave's farbling code uses (see `BraveSessionCache` and
// `BraveFarblingService`), seeded from the origin's persistent farbling token.
using FarblingPRNG = absl::random_internal::randen_engine<uint64_t>;

constexpr double kMaxUInt64AsDouble = static_cast<double>(UINT64_MAX);

// The parts that are combined to form a randomly generated fake plugin name.
// An empty entry represents an omitted part.
constexpr auto kPluginNameFirstParts = base::MakeFixedFlatSet<std::string_view>(
    {"Chrome", "Chromium", "Brave", "Web", "Browser", "OpenSource", "Online",
     "JavaScript", "WebKit", "Web-Kit", "WK", ""});

constexpr auto kPluginNameSecondParts =
    base::MakeFixedFlatSet<std::string_view>(
        {"PDF", "Portable Document Format", "portable-document-format",
         "document", "doc", "PDF and PS", "com.adobe.pdf", ""});

constexpr auto kPluginNameThirdParts = base::MakeFixedFlatSet<std::string_view>(
    {"Viewer", "Renderer", "Display", "Plugin", "plug-in", "plug in",
     "extension", ""});

// A list of fake voice names used to generate a fake `SpeechSynthesizer` voice.
constexpr auto kFakeVoiceNames = base::MakeFixedFlatSet<std::string_view>(
    {"Hubert", "Vernon", "Rudolph", "Clayton", "Irving", "Wilson", "Alva",
     "Harley", "Beauregard", "Cleveland", "Cecil", "Reuben", "Sylvester",
     "Jasper"});

// Returns the next value of `prng` mapped to a double in [0, 1).
double NextDouble(FarblingPRNG& prng) {
  return static_cast<double>(prng()) / kMaxUInt64AsDouble;
}

// Returns a value in [lower_bound, upper_bound] drawn from `prng`
double NextDoubleInRange(FarblingPRNG& prng,
                         double lower_bound,
                         double upper_bound) {
  return lower_bound + NextDouble(prng) * (upper_bound - lower_bound);
}

// Returns an integer in [lower_bound, upper_bound] drawn from `prng`
int NextIntInRange(FarblingPRNG& prng, int lower_bound, int upper_bound) {
  const double size = static_cast<double>(upper_bound - lower_bound);
  return lower_bound + static_cast<int>(NextDouble(prng) * size);
}

// Returns a random element from `values` drawn from `prng`
template <typename Container>
std::string_view NextElement(FarblingPRNG& prng, const Container& values) {
  const int index =
      NextIntInRange(prng, 0, static_cast<int>(values.size()) - 1);
  return *(values.begin() + index);
}

// Generates a random plugin name by joining a prefix, middle and suffix where
// any of those parts may be empty. May result in an empty string.
std::string MakeRandomPluginName(FarblingPRNG& prng) {
  std::vector<std::string_view> parts;
  for (std::string_view part : {NextElement(prng, kPluginNameFirstParts),
                                NextElement(prng, kPluginNameSecondParts),
                                NextElement(prng, kPluginNameThirdParts)}) {
    if (!part.empty()) {
      parts.push_back(part);
    }
  }
  return base::JoinString(parts, " ");
}

// Generates the fake plugin data to be injected by farbling.ts. Returns an
// empty list when the plugin farbling feature is disabled.
base::ListValue MakeFakePluginData(FarblingPRNG& prng) {
  base::ListValue plugins;
  if (!base::FeatureList::IsEnabled(
          brave_shields::features::kBraveIOSEnableFarblingPlugins)) {
    return plugins;
  }

  // Generate 1 to 3 fake plugins.
  const int plugin_count = NextIntInRange(prng, 1, 3);
  for (int i = 0; i < plugin_count; ++i) {
    // Generate 1 to 3 fake mime types for this plugin.
    const int mime_type_count = NextIntInRange(prng, 1, 3);
    base::ListValue mime_types;
    for (int j = 0; j < mime_type_count; ++j) {
      base::DictValue mime_type;
      mime_type.Set("suffixes", "pdf");
      mime_type.Set("type", "application/pdf");
      mime_type.Set("description", MakeRandomPluginName(prng));
      mime_types.Append(std::move(mime_type));
    }

    base::DictValue plugin;
    plugin.Set("name", MakeRandomPluginName(prng));
    plugin.Set("filename", "");
    plugin.Set("description", MakeRandomPluginName(prng));
    plugin.Set("mimeTypes", std::move(mime_types));
    plugins.Append(std::move(plugin));
  }
  return plugins;
}

}  // namespace

base::DictValue MakeFarblingArgs(const base::Token& farbling_token) {
  // Seed the generator from the origin's persistent farbling token, matching
  // how the rest of Brave's farbling code derives its seed.
  FarblingPRNG prng(farbling_token.high() ^ farbling_token.low());

  base::DictValue args;
  // A value between 0.99 and 1 to fudge audio data. A value in this range keeps
  // the destination values within the expected [-1, 1] range while introducing
  // a small random change for fingerprinters.
  args.Set("fudgeFactor", NextDoubleInRange(prng, 0.99, 1.0));
  // A fake voice name used to add a fake `SpeechSynthesizer` voice.
  args.Set("fakeVoiceName", NextElement(prng, kFakeVoiceNames));
  // Fake data used to construct fake plugins.
  args.Set("fakePluginData", MakeFakePluginData(prng));
  // Used to pick a fake voice index in [0, voices.length). Must be in [0, 1).
  args.Set("randomVoiceIndexScale", NextDouble(prng));
  // Used to pick a value between 2 and `hardwareConcurrency`. Must be in [0,
  // 1).
  args.Set("randomHardwareIndexScale", NextDouble(prng));
  return args;
}

}  // namespace brave_shields
