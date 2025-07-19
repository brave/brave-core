// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_HOSTED_EXTENSIONS_H_
#define BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_HOSTED_EXTENSIONS_H_

#include <array>
#include <optional>

#include "base/containers/fixed_flat_map.h"
#include "extensions/common/extension_id.h"

namespace extensions_mv2 {

inline constexpr char kNoScriptId[] = "bgkmgpgeempochogfoddiobpbhdfgkdi";
inline constexpr char kUBlockId[] = "jcokkipkhhgiakinbnnplhkdbjbgcgpe";
inline constexpr char kUMatrixId[] = "fplfeajmkijmaeldaknocljmmoebdgmk";
inline constexpr char kAdGuardId[] = "ejoelgckfgogkoppbgkklbbjdkjdbmen";

inline constexpr char kCwsNoScriptId[] = "doojmbjmlfjjnbmnoijecmcbfeoakpjm";
inline constexpr char kCwsUBlockId[] = "cjpalhdlnbpafiamejdnhcphjbkeiagm";
inline constexpr char kCwsUMatrixId[] = "ogfcmafjalglgifnmanfmnieipoejdcf";
inline constexpr char kCwsAdGuardId[] = "bgnkhhnnamicmpeenaelnjfhikgbkllg";

inline constexpr auto kBraveHosted =
    base::MakeFixedFlatMap<std::string_view, std::string_view>(
        {{kNoScriptId, kCwsNoScriptId},
         {kUBlockId, kCwsUBlockId},
         {kUMatrixId, kCwsUMatrixId},
         {kAdGuardId, kCwsAdGuardId}});

inline constexpr auto kCwsHosted =
    base::MakeFixedFlatMap<std::string_view, std::string_view>(
        {{kCwsNoScriptId, kNoScriptId},
         {kCwsUBlockId, kUBlockId},
         {kCwsUMatrixId, kUMatrixId},
         {kCwsAdGuardId, kAdGuardId}});

// In future there can be more brave-hosted mv2 extensions than published on
// CWS.
static_assert(kBraveHosted.size() >= kCwsHosted.size());

consteval std::array<std::string_view, kBraveHosted.size()>
GetPreconfiguredManifestV2Extensions() {
  // This can be made more idiomatic once Chromium style allows
  // std::views::keys
  std::array<std::string_view, kBraveHosted.size()> result{};
  std::ranges::transform(kBraveHosted, result.begin(),
                         [](const auto& p) { return p.first; });
  return result;
}

inline constexpr auto kPreconfiguredManifestV2Extensions =
    GetPreconfiguredManifestV2Extensions();

static_assert(kPreconfiguredManifestV2Extensions.size() == kBraveHosted.size());

bool IsKnownMV2Extension(const extensions::ExtensionId& id);
bool IsKnownCwsMV2Extension(const extensions::ExtensionId& id);

std::optional<extensions::ExtensionId> GetBraveHostedExtensionId(
    const extensions::ExtensionId& cws_extension_id);

std::optional<extensions::ExtensionId> GetCwsExtensionId(
    const extensions::ExtensionId& brave_hosted_extension_id);
}  // namespace extensions_mv2

#endif  // BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_HOSTED_EXTENSIONS_H_
