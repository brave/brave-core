// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/containers_icon_generator.h"

#include <memory>
#include <utility>

#include "base/hash/hash.h"
#include "base/notreached.h"
#include "brave/components/containers/core/browser/temporary_container.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icon_types.h"

namespace containers {
namespace {

class ContainersIconImageSource : public gfx::CanvasImageSource {
 public:
  ContainersIconImageSource(const gfx::ImageSkia& icon_image,
                            SkColor background,
                            int dip_size,
                            int dip_icon_size)
      : gfx::CanvasImageSource(gfx::Size(dip_size, dip_size)),
        icon_image_(std::move(icon_image)),
        background_(background),
        dip_icon_size_(dip_icon_size) {}

  void Draw(gfx::Canvas* canvas) override {
    DrawBackground(canvas);
    DrawIcon(canvas);
  }

 private:
  void DrawBackground(gfx::Canvas* canvas) const {
    cc::PaintFlags flags;
    flags.setColor(background_);
    flags.setAntiAlias(true);
    const auto radius = size().width() / 2.0;
    canvas->DrawCircle(gfx::PointF(radius, radius), radius, flags);
  }

  void DrawIcon(gfx::Canvas* canvas) const {
    canvas->DrawImageInt(icon_image_, (size().width() - dip_icon_size_) / 2,
                         (size().width() - dip_icon_size_) / 2);
  }

  const gfx::ImageSkia icon_image_;
  const SkColor background_;
  const int dip_icon_size_;
};

// Symmetric identicon image generator for temporary containers.
class TemporaryContainerForegroundIconImageSource
    : public gfx::CanvasImageSource {
 public:
  TemporaryContainerForegroundIconImageSource(std::string_view container_id,
                                              int icon_size,
                                              SkColor color)
      : gfx::CanvasImageSource(gfx::Size(icon_size, icon_size)),
        seed_(base::PersistentHash(container_id)),
        color_(color) {}

  void Draw(gfx::Canvas* canvas) override {
    const uint32_t pattern = (seed_ ^ (seed_ >> 15)) & 0x7fff;

    constexpr int kGrid = 5;
    // Match the visual safe area used by 16x16 vector icons (~12px-ish content)
    // so temporary identicons feel balanced next to them.
    constexpr float kContentBoxRatio = 12.0f / 16.0f;
    // Keep a little spacing between tiles to avoid a too-heavy blob.
    constexpr float kCellFillRatio = 0.9f;
    const float icon_size = size().width();
    const float content_box_size = icon_size * kContentBoxRatio;
    const float content_box_inset = (icon_size - content_box_size) / 2.0f;
    const float cell = content_box_size / static_cast<float>(kGrid);
    const float cell_size = cell * kCellFillRatio;
    const float cell_inset = (cell - cell_size) / 2.0f;

    cc::PaintFlags cell_flags;
    cell_flags.setAntiAlias(true);
    cell_flags.setColor(color_);

    // One row per horizontal band in the 5x5 grid; y is the top of the cell
    // for this row inside the content box.
    for (int row = 0; row < kGrid; ++row) {
      const float y = content_box_inset + row * cell + cell_inset;
      // Only the left three columns are encoded in the pattern (15 bits);
      // the right two are filled by mirroring for left-right symmetry.
      for (int col = 0; col < 3; ++col) {
        const int bit_index = row * 3 + col;
        // Skip the cell if the bit is not set.
        if (((pattern >> bit_index) & 1u) == 0u) {
          continue;
        }

        // Render the cell and its mirror on the opposite side.
        const float x = content_box_inset + col * cell + cell_inset;
        canvas->DrawRect(gfx::RectF(x, y, cell_size, cell_size), cell_flags);

        // Mirror the cell into the opposite side.
        const int mirror_col = kGrid - 1 - col;
        if (mirror_col != col) {
          const float mirror_x =
              content_box_inset + mirror_col * cell + cell_inset;
          canvas->DrawRect(gfx::RectF(mirror_x, y, cell_size, cell_size),
                           cell_flags);
        }
      }
    }
  }

 private:
  const uint32_t seed_;
  const SkColor color_;
};

}  // namespace

const gfx::VectorIcon& GetVectorIconFromIconType(mojom::Icon icon) {
  switch (icon) {
    case mojom::Icon::kPersonal:
      return kLeoContainerPersonalIcon;
    case mojom::Icon::kWork:
      return kLeoContainerWorkIcon;
    case mojom::Icon::kShopping:
      return kLeoContainerShoppingIcon;
    case mojom::Icon::kSocial:
      return kLeoContainerSocialIcon;
    case mojom::Icon::kEvents:
      return kLeoContainerEventsIcon;
    case mojom::Icon::kBanking:
      return kLeoContainerBankingIcon;
    case mojom::Icon::kStar:
      return kLeoContainerStarIcon;
    case mojom::Icon::kTravel:
      return kLeoContainerTravelIcon;
    case mojom::Icon::kSchool:
      return kLeoContainerSchoolIcon;
    case mojom::Icon::kPrivate:
      return kLeoContainerPrivateIcon;
    case mojom::Icon::kMessaging:
      return kLeoContainerMessagingIcon;
  }

  if (icon < mojom::Icon::kMinValue || icon > mojom::Icon::kMaxValue) {
    // Since icon is read from prefs and can be synced from a newer version
    // with a new set of icons, we return a default icon for compatibility.
    return GetVectorIconFromIconType(mojom::Icon::kDefault);
  }

  NOTREACHED() << "Unknown icon type: " << static_cast<int>(icon);
}

gfx::ImageSkia GenerateContainerIcon(std::string_view container_id,
                                     mojom::Icon icon,
                                     SkColor background,
                                     int dip_size,
                                     int dip_icon_size,
                                     float scale_factor,
                                     const ui::ColorProvider* color_provider) {
  gfx::ImageSkia icon_image = GenerateContainerForegroundIcon(
      container_id, icon, dip_icon_size, scale_factor);
  auto image_source = std::make_unique<ContainersIconImageSource>(
      std::move(icon_image), background, dip_size, dip_icon_size);
  return gfx::ImageSkia(std::move(image_source), scale_factor);
}

gfx::ImageSkia GenerateContainerForegroundIcon(std::string_view container_id,
                                               mojom::Icon icon,
                                               int dip_icon_size,
                                               float scale_factor) {
  if (IsTemporaryContainerId(container_id)) {
    return GenerateTemporaryContainerForegroundIcon(
        container_id, SK_ColorWHITE, dip_icon_size, scale_factor);
  }
  return gfx::CreateVectorIcon(GetVectorIconFromIconType(icon), dip_icon_size,
                               SK_ColorWHITE);
}

gfx::ImageSkia GenerateTemporaryContainerForegroundIcon(
    std::string_view container_id,
    SkColor color,
    int icon_size,
    float scale_factor) {
  auto image_source =
      std::make_unique<TemporaryContainerForegroundIconImageSource>(
          container_id, icon_size, color);
  return gfx::ImageSkia(std::move(image_source), scale_factor);
}

}  // namespace containers
