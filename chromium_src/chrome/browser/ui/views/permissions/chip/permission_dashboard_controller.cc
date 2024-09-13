/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/permissions/chip/permission_dashboard_controller.h"

#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/permissions/chip/permission_chip_view.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

bool IsAutoplay(content_settings::PageSpecificContentSettings* content_settings,
                ContentSettingImageModel* model) {
  // If upstream adds other types to the dashboard we may need to update this
  // code.
  DCHECK(model->image_type() ==
         ContentSettingImageModel::ImageType::MEDIASTREAM);

  if (!content_settings) {
    return false;
  }

  content_settings::PageSpecificContentSettings::MicrophoneCameraState state =
      content_settings->GetMicrophoneCameraState();
  if (state.Has(
          content_settings::PageSpecificContentSettings::kMicrophoneAccessed) ||
      state.Has(
          content_settings::PageSpecificContentSettings::kCameraAccessed)) {
    return false;
  }

  return true;
}

std::u16string GetAutoplayIndicatorTitle() {
  return l10n_util::GetStringUTF16(IDS_BLOCKED_AUTOPLAY_TITLE);
}

}  // namespace

// PermissionDashboardController::Update shows permissions chip for
// ContentSettingImageModel::ImageType::MEDIASTREAM, which upstream limits to
// Camera and Microphone. We add AUTOPLAY to MEDIASTREAM, so we need to return
// the appropriate title for it. Update is called from LocationBarView's
// RefreshContentSettingViews.
#define SetMessage(...)                                                     \
  SetMessage(                                                               \
      IsAutoplay(                                                           \
          content_settings::PageSpecificContentSettings::GetForFrame(       \
              location_bar_view_->GetWebContents()->GetPrimaryMainFrame()), \
          indicator_model)                                                  \
          ? GetAutoplayIndicatorTitle()                                     \
          : GetIndicatorTitle(indicator_model))

#include "src/chrome/browser/ui/views/permissions/chip/permission_dashboard_controller.cc"
#undef SetMessage
