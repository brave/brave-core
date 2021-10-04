// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "ui/views/layout/box_layout.h"

#include "../../../../../ui/views/window/dialog_client_view.cc"

namespace views {

namespace {

constexpr int kVerticalButtonsSpacing = 8;

}  // namespace

void DialogClientView::SetupButtonsLayoutVertically() {
  button_row_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, button_row_insets_,
      kVerticalButtonsSpacing));
}

}  // namespace views
