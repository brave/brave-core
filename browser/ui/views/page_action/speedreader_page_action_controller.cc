// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/speedreader_page_action_controller.h"

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event_constants.h"

namespace page_actions {

namespace {

constexpr int kIconSize = 16;

}  // namespace

SpeedreaderPageActionController::SpeedreaderPageActionController(
    tabs::TabInterface& tab,
    page_actions::PageActionController& page_action_controller)
    : tab_(tab),
      page_action_controller_(
          static_cast<page_actions::PageActionControllerImpl&>(
              page_action_controller)) {
  CHECK(
      base::FeatureList::IsEnabled(speedreader::features::kSpeedreaderFeature));
}

SpeedreaderPageActionController::~SpeedreaderPageActionController() = default;

void SpeedreaderPageActionController::Init() {
  did_activate_subscription_ = tab_->RegisterDidActivate(base::BindRepeating(
      [](SpeedreaderPageActionController* self, tabs::TabInterface*) {
        self->AttachToTabHelper();
        self->UpdatePageAction();
      },
      base::Unretained(this)));
  will_discard_contents_subscription_ =
      tab_->RegisterWillDiscardContents(base::BindRepeating(
          [](SpeedreaderPageActionController* self, tabs::TabInterface*,
             content::WebContents*, content::WebContents*) {
            self->AttachToTabHelper();
            self->UpdatePageAction();
          },
          base::Unretained(this)));
  AttachToTabHelper();
  UpdatePageAction();
}

void SpeedreaderPageActionController::ExecuteAction(int event_flags) {
  content::WebContents* const contents = tab_->GetContents();
  if (!contents) {
    return;
  }
  auto* tab_helper =
      speedreader::SpeedreaderTabHelper::FromWebContents(contents);
  if (!tab_helper) {
    return;
  }

  if (event_flags & ui::EF_RIGHT_MOUSE_BUTTON) {
    tab_helper->ShowSpeedreaderBubble(
        speedreader::SpeedreaderBubbleLocation::kLocationBar);
    return;
  }
  tab_helper->ProcessIconClick();
}

void SpeedreaderPageActionController::OnDistillStateUpdated() {
  UpdatePageAction();
}

void SpeedreaderPageActionController::AttachToTabHelper() {
  tab_helper_observation_.Reset();
  if (content::WebContents* contents = tab_->GetContents()) {
    if (auto* tab_helper =
            speedreader::SpeedreaderTabHelper::FromWebContents(contents)) {
      tab_helper_observation_.Observe(tab_helper);
    }
  }
}

void SpeedreaderPageActionController::UpdatePageAction() {
  content::WebContents* const contents = tab_->GetContents();
  if (!contents) {
    page_action_controller_->Hide(kActionShowSpeedreader);
    return;
  }

  auto* profile = Profile::FromBrowserContext(contents->GetBrowserContext());
  if (!base::FeatureList::IsEnabled(
          speedreader::features::kSpeedreaderFeature) ||
      !profile->GetPrefs()->GetBoolean(speedreader::kSpeedreaderEnabled)) {
    page_action_controller_->Hide(kActionShowSpeedreader);
    return;
  }

  auto* tab_helper =
      speedreader::SpeedreaderTabHelper::FromWebContents(contents);
  if (!tab_helper) {
    page_action_controller_->Hide(kActionShowSpeedreader);
    return;
  }

  const auto state = tab_helper->PageDistillState();
  const bool is_distilled = speedreader::DistillStates::IsDistilled(state);
  if (!is_distilled && !speedreader::DistillStates::IsDistillable(state)) {
    page_action_controller_->Hide(kActionShowSpeedreader);
    return;
  }

  page_action_controller_->Show(kActionShowSpeedreader);
  page_action_controller_->SetOverrideTriggerableEvent(
      kActionShowSpeedreader,
      ui::EF_LEFT_MOUSE_BUTTON | ui::EF_RIGHT_MOUSE_BUTTON);

  const std::u16string tooltip = l10n_util::GetStringUTF16(
      is_distilled ? IDS_SPEEDREADER_ICON_TURN_OFF_READER_MODE
                   : IDS_SPEEDREADER_ICON_TURN_ON_READER_MODE);
  page_action_controller_->OverrideTooltip(kActionShowSpeedreader, tooltip);
  page_action_controller_->OverrideAccessibleName(kActionShowSpeedreader,
                                                  tooltip);

  const ui::ColorId icon_color_id = is_distilled
                                        ? ui::ColorId(kColorSpeedreaderIcon)
                                        : ui::ColorId(kColorOmniboxResultsIcon);
  const SkColor icon_color =
      contents->GetColorProvider().GetColor(icon_color_id);
  page_action_controller_->OverrideImage(
      kActionShowSpeedreader,
      ui::ImageModel::FromVectorIcon(kLeoProductSpeedreaderIcon, icon_color,
                                     kIconSize));
}

}  // namespace page_actions
