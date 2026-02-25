// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/partitioned_storage_page_action_controller.h"

#include <algorithm>
#include <optional>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/containers/container_model.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "content/public/browser/web_contents.h"

namespace page_actions {

namespace {

constexpr float kDefaultScaleFactor = 1.0f;

std::optional<containers::ContainerModel> GetContainerModelForWebContents(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return std::nullopt;
  }
  std::string container_id =
      containers::GetContainerIdForWebContents(web_contents);
  if (container_id.empty()) {
    return std::nullopt;
  }
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  if (!profile) {
    return std::nullopt;
  }
  std::vector<containers::ContainerModel> models =
      containers::GetContainerModelsFromPrefs(*profile->GetPrefs(),
                                              kDefaultScaleFactor);
  auto it =
      std::ranges::find(models, container_id, &containers::ContainerModel::id);
  if (it != models.end()) {
    return containers::ContainerModel(std::move(*it));
  }

  return containers::ContainerModel::CreateForUnknown(container_id,
                                                      kDefaultScaleFactor);
}

}  // namespace

PartitionedStoragePageActionController::PartitionedStoragePageActionController(
    tabs::TabInterface& tab,
    page_actions::PageActionController& page_action_controller)
    : tab_(tab), page_action_controller_(page_action_controller) {
  CHECK(base::FeatureList::IsEnabled(containers::features::kContainers));
}

PartitionedStoragePageActionController::
    ~PartitionedStoragePageActionController() = default;

void PartitionedStoragePageActionController::Init() {
  did_activate_subscription_ = tab_->RegisterDidActivate(
      base::BindRepeating([](PartitionedStoragePageActionController* self,
                             tabs::TabInterface*) { self->UpdatePageAction(); },
                          base::Unretained(this)));
  did_become_visible_subscription_ = tab_->RegisterDidBecomeVisible(
      base::BindRepeating([](PartitionedStoragePageActionController* self,
                             tabs::TabInterface*) { self->UpdatePageAction(); },
                          base::Unretained(this)));
  will_discard_contents_subscription_ =
      tab_->RegisterWillDiscardContents(base::BindRepeating(
          [](PartitionedStoragePageActionController* self, tabs::TabInterface*,
             content::WebContents*,
             content::WebContents*) { self->UpdatePageAction(); },
          base::Unretained(this)));
  UpdatePageAction();
}

void PartitionedStoragePageActionController::UpdatePageAction() {
  content::WebContents* web_contents = tab_->GetContents();
  std::optional<containers::ContainerModel> model =
      GetContainerModelForWebContents(web_contents);
  if (!model) {
    page_action_controller_->Hide(kActionShowPartitionedStorage);
    page_action_controller_->ClearOverrideChipColors(
        kActionShowPartitionedStorage);
    return;
  }

  const std::u16string name = base::UTF8ToUTF16(model->name());
  page_action_controller_->Show(kActionShowPartitionedStorage);
  page_action_controller_->ShowSuggestionChip(kActionShowPartitionedStorage);
  page_action_controller_->SetAlwaysShowLabel(kActionShowPartitionedStorage,
                                              true);
  page_action_controller_->OverrideImage(kActionShowPartitionedStorage,
                                         model->icon());
  page_action_controller_->OverrideText(kActionShowPartitionedStorage, name);
  page_action_controller_->OverrideAccessibleName(kActionShowPartitionedStorage,
                                                  name);
  page_action_controller_->OverrideTooltip(kActionShowPartitionedStorage, name);
  page_action_controller_->OverrideChipColors(
      kActionShowPartitionedStorage, model->background_color(), SK_ColorWHITE);
}

}  // namespace page_actions
