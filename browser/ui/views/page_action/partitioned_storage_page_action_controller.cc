// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/partitioned_storage_page_action_controller.h"

#include <algorithm>
#include <optional>

#include "base/i18n/char_iterator.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/containers/containers_service_factory.h"
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

  auto* service = ContainersServiceFactory::GetForProfile(
      Profile::FromBrowserContext(web_contents->GetBrowserContext()));
  if (!service) {
    return std::nullopt;
  }

  return containers::GetRuntimeContainerModel(*service, container_id,
                                              kDefaultScaleFactor);
}

// Truncate the string to the given number of characters. This function is
// needed because we can have multi-byte characters in the string, so we
// should not truncate the surrogate pairs.
std::u16string TruncateString16(const std::u16string& input, size_t max_chars) {
  CHECK_GT(max_chars, 0u);
  base::i18n::UTF16CharIterator iter(input);
  iter.Advance();
  --max_chars;
  while (max_chars > 0) {
    iter.Advance();
    --max_chars;
  }
  return input.substr(0, iter.array_pos());
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
    page_action_controller_->ClearOverrideHeight(kActionShowPartitionedStorage);
    return;
  }

  const std::u16string name = base::UTF8ToUTF16(model->name());
  page_action_controller_->Show(kActionShowPartitionedStorage);
  page_action_controller_->ShowSuggestionChip(kActionShowPartitionedStorage);
  page_action_controller_->SetAlwaysShowLabel(kActionShowPartitionedStorage,
                                              true);
  page_action_controller_->OverrideImage(kActionShowPartitionedStorage,
                                         model->icon());

  // So far, we didn't have any limit for the name length, so if we don't
  // truncate the name, it will make url invisible because the PageActinView
  // will be too wide.
  const auto truncated_name = TruncateString16(name, 20);
  page_action_controller_->OverrideText(kActionShowPartitionedStorage,
                                        truncated_name);

  page_action_controller_->OverrideAccessibleName(kActionShowPartitionedStorage,
                                                  name);
  page_action_controller_->OverrideTooltip(kActionShowPartitionedStorage, name);
  page_action_controller_->OverrideChipColors(
      kActionShowPartitionedStorage, model->background_color(), SK_ColorWHITE);
  page_action_controller_->SetOverrideHeight(kActionShowPartitionedStorage, 20);
}

}  // namespace page_actions
