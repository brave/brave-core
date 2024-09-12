// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/views/controls/menu/menu_config.h"

#include "ui/views/controls/menu/menu_controller.h"

#define instance instance_ChromiumImpl

#include "src/ui/views/controls/menu/menu_config.cc"

#undef instance

namespace views {

MenuConfig::MenuConfig(const MenuConfig&) = default;

// static
const MenuConfig& MenuConfig::instance() {
  const auto& config = instance_ChromiumImpl();
  static class RunOnce {
   public:
    RunOnce(const MenuConfig& config) {
      auto& mutable_config = const_cast<MenuConfig&>(config);
      // Each platform sets its own config in its Init().
      // Apply our config globally at once after Init() is done.
      mutable_config.item_horizontal_border_padding = 4;
      mutable_config.item_horizontal_padding =
          24 - config.item_horizontal_border_padding;
      mutable_config.corner_radius = 8;
      mutable_config.use_bubble_border = true;
    }
  } const run_once(config);

  return config;
}

}  // namespace views
