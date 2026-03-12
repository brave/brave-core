// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/accent_color/brave_tab_accent_color_palette.h"

#include <algorithm>
#include <array>
#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/gfx/color_palette.h"

// Macro to get num suffixed color id. ex) Primitive40(Blue) ->
// kColorPrimitiveBlue40 The resulting color id is enum in nala_color_id.h
#define COLOR_10(color) nala::kColor##color##10
#define COLOR_20(color) nala::kColor##color##20
#define COLOR_40(color) nala::kColor##color##40
#define PRIMITIVE_60(color) nala::kColorPrimitive##color##60
#define COLOR_KEY_AND_COLOR_ID_PAIR(color) \
  std::pair {                              \
    k##color, PRIMITIVE_60(color)          \
  }

// Yield color ids for dark-pinned-active
#define DARK_PINNED_ACTIVE_COLOR_IDS(color)     \
  ColorIds {                                    \
    .icon_color_id = nala::kColorWhite,         \
    .icon_border_color_id = COLOR_40(color),    \
    .background_color_id = PRIMITIVE_60(color), \
    .border_color_id = PRIMITIVE_60(color),     \
  }

// Yield color ids for dark-pinned-inactive
#define DARK_PINNED_INACTIVE_COLOR_IDS(color)                                  \
  ColorIds {                                                                   \
    .icon_color_id = COLOR_40(color), .icon_border_color_id = COLOR_20(color), \
    .background_color_id = COLOR_10(color), .border_color_id = 0,              \
  }

// Yield color ids for dark-pinned-hovered
#define DARK_PINNED_HOVERED_COLOR_IDS(color)                                   \
  ColorIds {                                                                   \
    .icon_color_id = COLOR_40(color), .icon_border_color_id = COLOR_20(color), \
    .background_color_id = COLOR_10(color), .border_color_id = 0,              \
    .override_tab_background_color_id = COLOR_10(color),                       \
  }

// Yield color ids for dark-unpinned-active
#define DARK_UNPINNED_ACTIVE_COLOR_IDS(color)    \
  ColorIds {                                     \
    .icon_color_id = nala::kColorWhite,          \
    .icon_border_color_id = PRIMITIVE_60(color), \
    .background_color_id = PRIMITIVE_60(color),  \
    .border_color_id = PRIMITIVE_60(color),      \
  }

// Yield color ids for dark-unpinned-inactive
#define DARK_UNPINNED_INACTIVE_COLOR_IDS(color) \
  ColorIds {                                    \
    .icon_color_id = PRIMITIVE_60(color),       \
    .icon_border_color_id = COLOR_10(color),    \
    .background_color_id = COLOR_10(color),     \
    .border_color_id = COLOR_10(color),         \
  }

// Yield color ids for dark-unpinned-hovered
#define DARK_UNPINNED_HOVERED_COLOR_IDS(color)           \
  ColorIds {                                             \
    .icon_color_id = PRIMITIVE_60(color),                \
    .icon_border_color_id = COLOR_20(color),             \
    .background_color_id = COLOR_20(color),              \
    .border_color_id = COLOR_20(color),                  \
    .override_tab_background_color_id = COLOR_10(color), \
  }

// Yield color ids for light-pinned-active
#define LIGHT_PINNED_ACTIVE_COLOR_IDS(color)    \
  ColorIds {                                    \
    .icon_color_id = nala::kColorWhite,         \
    .icon_border_color_id = COLOR_40(color),    \
    .background_color_id = PRIMITIVE_60(color), \
    .border_color_id = PRIMITIVE_60(color),     \
  }

// Yield color ids for light-pinned-inactive
#define LIGHT_PINNED_INACTIVE_COLOR_IDS(color)                                 \
  ColorIds {                                                                   \
    .icon_color_id = COLOR_40(color), .icon_border_color_id = COLOR_20(color), \
    .background_color_id = COLOR_10(color), .border_color_id = 0,              \
  }

// Yield color ids for light-pinned-hovered
#define LIGHT_PINNED_HOVERED_COLOR_IDS(color)                                  \
  ColorIds {                                                                   \
    .icon_color_id = COLOR_40(color), .icon_border_color_id = COLOR_20(color), \
    .background_color_id = COLOR_10(color), .border_color_id = 0,              \
    .override_tab_background_color_id = COLOR_10(color),                       \
  }

// Yield color ids for light-unpinned-active
#define LIGHT_UNPINNED_ACTIVE_COLOR_IDS(color)   \
  ColorIds {                                     \
    .icon_color_id = nala::kColorWhite,          \
    .icon_border_color_id = PRIMITIVE_60(color), \
    .background_color_id = PRIMITIVE_60(color),  \
    .border_color_id = PRIMITIVE_60(color),      \
  }

// Yield color ids for light-unpinned-hovered
#define LIGHT_UNPINNED_HOVERED_COLOR_IDS(color)          \
  ColorIds {                                             \
    .icon_color_id = PRIMITIVE_60(color),                \
    .icon_border_color_id = COLOR_20(color),             \
    .background_color_id = COLOR_20(color),              \
    .border_color_id = COLOR_20(color),                  \
    .override_tab_background_color_id = COLOR_10(color), \
  }

// Yield color ids for light-unpinned-inactive
#define LIGHT_UNPINNED_INACTIVE_COLOR_IDS(color) \
  ColorIds {                                     \
    .icon_color_id = PRIMITIVE_60(color),        \
    .icon_border_color_id = COLOR_10(color),     \
    .background_color_id = COLOR_10(color),      \
    .border_color_id = COLOR_10(color),          \
  }

// [state]: active, hovered, inactive (each wrapped so one subobject)
#define DARK_PINNED_COLOR_IDS(color)           \
  {                                            \
      {DARK_PINNED_ACTIVE_COLOR_IDS(color)},   \
      {DARK_PINNED_HOVERED_COLOR_IDS(color)},  \
      {DARK_PINNED_INACTIVE_COLOR_IDS(color)}, \
  }
#define DARK_UNPINNED_COLOR_IDS(color)           \
  {                                              \
      {DARK_UNPINNED_ACTIVE_COLOR_IDS(color)},   \
      {DARK_UNPINNED_HOVERED_COLOR_IDS(color)},  \
      {DARK_UNPINNED_INACTIVE_COLOR_IDS(color)}, \
  }
#define LIGHT_PINNED_COLOR_IDS(color)           \
  {                                             \
      {LIGHT_PINNED_ACTIVE_COLOR_IDS(color)},   \
      {LIGHT_PINNED_HOVERED_COLOR_IDS(color)},  \
      {LIGHT_PINNED_INACTIVE_COLOR_IDS(color)}, \
  }
#define LIGHT_UNPINNED_COLOR_IDS(color)           \
  {                                               \
      {LIGHT_UNPINNED_ACTIVE_COLOR_IDS(color)},   \
      {LIGHT_UNPINNED_HOVERED_COLOR_IDS(color)},  \
      {LIGHT_UNPINNED_INACTIVE_COLOR_IDS(color)}, \
  }

// [pinned]: unpinned=0, pinned=1 (each wrapped)
#define DARK_COLOR_IDS(color)           \
  {                                     \
    {DARK_UNPINNED_COLOR_IDS(color)}, { \
      DARK_PINNED_COLOR_IDS(color)      \
    }                                   \
  }
#define LIGHT_COLOR_IDS(color)           \
  {                                      \
    {LIGHT_UNPINNED_COLOR_IDS(color)}, { \
      LIGHT_PINNED_COLOR_IDS(color)      \
    }                                    \
  }

// [theme]: light=0, dark=1 (each wrapped). Single expression for map value.
#define COLOR_IDS_FOR(color)    \
  {                             \
    {LIGHT_COLOR_IDS(color)}, { \
      DARK_COLOR_IDS(color)     \
    }                           \
  }

namespace accent_color {

namespace {

struct ColorIds {
  ui::ColorId icon_color_id = 0;
  ui::ColorId icon_border_color_id = 0;
  ui::ColorId background_color_id = 0;
  ui::ColorId border_color_id = 0;
  ui::ColorId override_tab_background_color_id = 0;
};

enum ColorKey {
  kRed = 0,
  kOrange,
  kYellow,
  kGreen,
  kTeal,
  kBlue,
  kPurple,
  kPink,
  kNum = kPink,
};

// [theme: light=0, dark=1][pinned: unpinned=0, pinned=1][state: active=0,
// inactive=1, hovered=2].
using ColorIdsByThemePinnedState = std::array<
    std::array<std::array<ColorIds,
                          static_cast<int>(TabAccentColorsParams::State::kNum)>,
               2>,
    2>;

const auto& GetColorIdsMap() {
  static constexpr auto kColorIdsMap =
      base::MakeFixedFlatMap<ColorKey, ColorIdsByThemePinnedState>({
          {kRed, ColorIdsByThemePinnedState{COLOR_IDS_FOR(Red)}},
          {kOrange, ColorIdsByThemePinnedState{COLOR_IDS_FOR(Orange)}},
          {kYellow, ColorIdsByThemePinnedState{COLOR_IDS_FOR(Yellow)}},
          {kGreen, ColorIdsByThemePinnedState{COLOR_IDS_FOR(Green)}},
          {kTeal, ColorIdsByThemePinnedState{COLOR_IDS_FOR(Teal)}},
          {kBlue, ColorIdsByThemePinnedState{COLOR_IDS_FOR(Blue)}},
          {kPurple, ColorIdsByThemePinnedState{COLOR_IDS_FOR(Purple)}},
          {kPink, ColorIdsByThemePinnedState{COLOR_IDS_FOR(Pink)}},
      });
  return kColorIdsMap;
}

// Finds the nearest palette key by Euclidean distance in RGB (using
// PRIMITIVE_60 reference colors from the color provider).
ColorKey FindBestKey(SkColor container_color,
                     const ui::ColorProvider* color_provider) {
  static constexpr std::array kNearestColorCandidates = {
      COLOR_KEY_AND_COLOR_ID_PAIR(Red),    COLOR_KEY_AND_COLOR_ID_PAIR(Orange),
      COLOR_KEY_AND_COLOR_ID_PAIR(Yellow), COLOR_KEY_AND_COLOR_ID_PAIR(Green),
      COLOR_KEY_AND_COLOR_ID_PAIR(Purple), COLOR_KEY_AND_COLOR_ID_PAIR(Pink),
  };

  auto sq_distance = [container_color](SkColor other) {
    const int dr = SkColorGetR(container_color) - SkColorGetR(other);
    const int dg = SkColorGetG(container_color) - SkColorGetG(other);
    const int db = SkColorGetB(container_color) - SkColorGetB(other);
    return dr * dr + dg * dg + db * db;
  };

  const auto best = std::ranges::min_element(
      kNearestColorCandidates, std::less{}, [&](const auto& key_and_color_id) {
        return sq_distance(color_provider->GetColor(key_and_color_id.second));
      });
  return best->first;
}

}  // namespace

TabAccentColors GetTabAccentColors(const TabAccentColorsParams& params,
                                   const ui::ColorProvider* color_provider) {
  if (!color_provider) {
    return {};
  }

  // This should be updated when nala token's colors are updated.
  // Do not remove the old values, as users could be using the old values by
  // the preference.
  ColorKey key;
  switch (params.container_color) {
    case SkColorSetRGB(0xB7, 0x4D, 0x49):  // kColorPrimitiveRed50:26/03/03
      key = kRed;
      break;
    case SkColorSetRGB(0xB2, 0x54, 0x2E):  // kColorPrimitiveOrange50:26/03/03
      key = kOrange;
      break;
    case SkColorSetRGB(0x8E, 0x67, 0x02):  // kColorPrimitiveYellow50:26/03/03
      key = kYellow;
      break;
    case SkColorSetRGB(0x1F, 0x7E, 0x48):  // kColorPrimitiveGreen50:26/03/03
      key = kGreen;
      break;
    case SkColorSetRGB(0x00, 0x7B, 0x92):  // kColorPrimitiveTeal50:26/03/03
      key = kTeal;
      break;
    case SkColorSetRGB(0x36, 0x6E, 0xB8):  // kColorPrimitiveBlue50:26/03/03
      key = kBlue;
      break;
    case SkColorSetRGB(0x7C, 0x5D, 0xAF):  // kColorPrimitivePurple50:26/03/03
      key = kPurple;
      break;
    case SkColorSetRGB(0xB2, 0x4D, 0x71):  // kColorPrimitivePink50:26/03/03
      key = kPink;
      break;
    default:
      // Find the nearest palette key. This can happen due to preferences
      // synchronization or Nala updates.
      key = FindBestKey(params.container_color, color_provider);
      break;
  }

  const auto& color_ids_map = GetColorIdsMap();
  CHECK(color_ids_map.contains(key));
  const auto& color_ids = color_ids_map.at(key);
  const ColorIds& ids = color_ids[params.is_dark][params.is_pinned]
                                 [static_cast<int>(params.state)];
  return {
      .border_color = ids.border_color_id == 0
                          ? SK_ColorTRANSPARENT
                          : color_provider->GetColor(ids.border_color_id),
      .background_color =
          ids.background_color_id == 0
              ? SK_ColorTRANSPARENT
              : color_provider->GetColor(ids.background_color_id),
      .icon_color = ids.icon_color_id == 0
                        ? SK_ColorTRANSPARENT
                        : color_provider->GetColor(ids.icon_color_id),
      .icon_border_color =
          ids.icon_border_color_id == 0
              ? SK_ColorTRANSPARENT
              : color_provider->GetColor(ids.icon_border_color_id),
      .override_tab_background_color =
          ids.override_tab_background_color_id == 0
              ? SK_ColorTRANSPARENT
              : color_provider->GetColor(ids.override_tab_background_color_id),
  };
}

}  // namespace accent_color
