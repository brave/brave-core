// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <utility>

#include "src/chrome/browser/ui/views/toolbar/toolbar_button.cc"

void ToolbarButton::SetMenuModel(std::unique_ptr<ui::MenuModel> model) {
  model_ = std::move(model);
}
