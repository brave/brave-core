/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/toolbar/app_menu.h"

#include <memory>

#include "chrome/browser/ui/views/frame/app_menu_button.h"

#include <chrome/browser/ui/views/toolbar/app_menu.cc>

std::unique_ptr<views::Background>
AppMenu::CreateInMenuButtonBackgroundWithLeadingBorder() {
  return std::make_unique<InMenuButtonBackground>(
      InMenuButtonBackground::ButtonType::kLeadingBorder);
}
