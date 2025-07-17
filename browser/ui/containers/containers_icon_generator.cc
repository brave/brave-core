// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/containers_icon_generator.h"

#include <memory>
#include <utility>

#include "base/notreached.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icon_types.h"

namespace containers {
namespace {

class ContainersIconImageSource : public gfx::CanvasImageSource {
 public:
  ContainersIconImageSource(mojom::Icon icon, SkColor background, int dip_size)
      : gfx::CanvasImageSource(gfx::Size(dip_size, dip_size)),
        icon_(icon),
        background_(background),
        dip_size_(dip_size) {}

  void Draw(gfx::Canvas* canvas) override {
    DrawBackground(canvas);
    DrawIcon(canvas);
  }

 private:
  void DrawBackground(gfx::Canvas* canvas) const {
    cc::PaintFlags flags;
    flags.setColor(background_);
    flags.setAntiAlias(true);
    const auto radius = dip_size_ / 2.0;
    canvas->DrawCircle(gfx::PointF(radius, radius), radius, flags);
  }

  void DrawIcon(gfx::Canvas* canvas) const {
    gfx::PaintVectorIcon(canvas, GetVectorIconFromIconType(icon_), dip_size_,
                         SK_ColorWHITE);
  }

  const mojom::Icon icon_;
  const SkColor background_;
  const int dip_size_;
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

gfx::ImageSkia GenerateContainerIcon(mojom::Icon icon,
                                     SkColor background,
                                     int dip_size,
                                     float scale_factor,
                                     const ui::ColorProvider* color_provider) {
  auto image_source =
      std::make_unique<ContainersIconImageSource>(icon, background, dip_size);
  return gfx::ImageSkia(std::move(image_source), scale_factor);
}

}  // namespace containers
