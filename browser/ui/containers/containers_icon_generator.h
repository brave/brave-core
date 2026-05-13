// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_ICON_GENERATOR_H_
#define BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_ICON_GENERATOR_H_

#include <string_view>

#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"

namespace gfx {
struct VectorIcon;
}

namespace ui {
class ColorProvider;
}  // namespace ui

namespace containers {

// This is a function to get the vector icon corresponding to the
// mojom::Icon type.
const gfx::VectorIcon& GetVectorIconFromIconType(mojom::Icon icon);

// This is a function to generate icons for Containers, which can be used in
// ui::ImageModel directly. Typical use cases of this function include context
// menus, tab indicators, page actions, and etc.
gfx::ImageSkia GenerateContainerIcon(std::string_view container_id,
                                     mojom::Icon icon,
                                     SkColor background,
                                     int dip_size,
                                     int dip_icon_size,
                                     float scale_factor,
                                     const ui::ColorProvider* color_provider);

// Returns a foreground icon for the container. In case the container_id is a
// temporary container, it will return a temporary container foreground icon.
// Otherwise, it will return a vector icon for the container.
gfx::ImageSkia GenerateContainerForegroundIcon(std::string_view container_id,
                                               mojom::Icon icon,
                                               int dip_icon_size,
                                               float scale_factor);

// Generates a transparent image containing only the temporary container
// foreground identicon centered in |icon_size|.
gfx::ImageSkia GenerateTemporaryContainerForegroundIcon(
    std::string_view container_id,
    SkColor color,
    int icon_size,
    float scale_factor);

}  // namespace containers

#endif  // BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_ICON_GENERATOR_H_
