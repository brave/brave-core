/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/device_chooser_content_view.h"

#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "components/permissions/chooser_controller.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view_class_properties.h"

namespace {

void AddBluetoothWarningMessage(
    DeviceChooserContentView* view,
    permissions::ChooserController* chooser_controller) {
  DCHECK(view);
  DCHECK(chooser_controller);
  if (chooser_controller->GetType() !=
      permissions::ChooserControllerType::kBluetooth) {
    return;
  }

  ChromeLayoutProvider* layout_provider = ChromeLayoutProvider::Get();
  view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  auto* label = new views::Label(
      l10n_util::GetStringUTF16(
          IDS_PERMISSIONS_BLUETOOTH_CHOOSER_PRIVACY_WARNING_TEXT),
      views::style::CONTEXT_LABEL, views::style::STYLE_EMPHASIZED);
  label->SetMultiLine(true);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetProperty(
      views::kMarginsKey,
      gfx::Insets::TLBR(0, 0,
                        layout_provider->GetDistanceMetric(
                            views::DISTANCE_RELATED_CONTROL_VERTICAL),
                        0));
  view->AddChildView(label);
}

}  // namespace

#define SetUseDefaultFillLayout(...)    \
  SetUseDefaultFillLayout(__VA_ARGS__); \
  AddBluetoothWarningMessage(this, chooser_controller_.get())

#include "src/chrome/browser/ui/views/device_chooser_content_view.cc"

#undef SetUseDefaultFillLayout
