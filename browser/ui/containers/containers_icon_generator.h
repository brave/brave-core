// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_ICON_GENERATOR_H_
#define BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_ICON_GENERATOR_H_

#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"

namespace ui {
class ColorProvider;
}  // namespace ui

namespace containers {

// This is a functor to generate icons for Containers, which can be used in
// ui::ImageModel directly. Typical use cases of this functor include context
// menus, tab indicators, page actions, and etc.
struct ContainersIconGenerator {
  ContainersIconGenerator(mojom::Icon icon,
                          SkColor background,
                          int dip_size,
                          float scale_factor);
  ~ContainersIconGenerator() = default;

  gfx::ImageSkia operator()(const ui::ColorProvider* color_provider) const;

  const mojom::Icon icon;
  const SkColor background;
  const int dip_size;
  const float scale_factor;
};

}  // namespace containers

#endif  // BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_ICON_GENERATOR_H_
