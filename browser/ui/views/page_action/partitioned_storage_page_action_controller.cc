// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/partitioned_storage_page_action_controller.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/browser/ui/containers/container_model.h"
#include "brave/browser/ui/containers/containers_menu_model.h"
#include "brave/browser/ui/tabs/containers_tab_menu_model_delegate.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/location_bar/icon_label_bubble_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/actions/actions.h"
#include "ui/base/mojom/menu_source_type.mojom.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/text_elider.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"

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

}  // namespace

PartitionedStoragePageActionController::PartitionedStoragePageActionController(
    tabs::TabInterface& tab,
    page_actions::PageActionController& page_action_controller)
    : tab_(tab), page_action_controller_(page_action_controller) {
  CHECK(base::FeatureList::IsEnabled(containers::features::kContainers));
}

PartitionedStoragePageActionController::
    ~PartitionedStoragePageActionController() {
  // |MenuRunner| must outlive the open menu (|RunMenuAt| returns while the
  // menu is still running). Hold it in |menu_runner_| until close. Reset here
  // so teardown runs while this object is still valid.
  weak_ptr_factory_.InvalidateWeakPtrs();
  menu_runner_.reset();
  menu_anchor_view_tracker_.SetView(nullptr);
  if (action_item_for_menu_) {
    action_item_for_menu_->SetIsShowingBubble(false);
    action_item_for_menu_ = nullptr;
  }
  menu_model_adapter_.reset();
  containers_menu_model_.reset();
  containers_menu_delegate_.reset();
}

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

void PartitionedStoragePageActionController::ExecuteAction(
    ToolbarButtonProvider* toolbar_button_provider,
    actions::ActionItem* item) {
  if (menu_runner_ && menu_runner_->IsRunning()) {
    menu_runner_->Cancel();
  }

  if (!toolbar_button_provider) {
    return;
  }

  BrowserWindowInterface* const bwi = tab_->GetBrowserWindowInterface();
  if (!bwi) {
    return;
  }

  Profile* const profile = bwi->GetProfile();
  containers::ContainersService* const service =
      ContainersServiceFactory::GetForProfile(profile);
  if (!service) {
    return;
  }

  IconLabelBubbleView* const anchor_view =
      toolbar_button_provider->GetPageActionView(kActionShowPartitionedStorage);
  if (!anchor_view || !anchor_view->GetWidget()) {
    return;
  }

  containers_menu_delegate_ =
      std::make_unique<brave::ContainersTabMenuModelDelegate>(
          bwi, std::vector<tabs::TabHandle>{tab_->GetHandle()});
  containers_menu_model_ = std::make_unique<containers::ContainersMenuModel>(
      *containers_menu_delegate_, *service);

  menu_anchor_view_tracker_.SetView(anchor_view);
  action_item_for_menu_ = item;
  CHECK_DEREF(action_item_for_menu_).SetIsShowingBubble(true);

  menu_model_adapter_ = std::make_unique<views::MenuModelAdapter>(
      containers_menu_model_.get(),
      base::BindRepeating(&PartitionedStoragePageActionController::
                              OnPartitionedStorageMenuClosed,
                          weak_ptr_factory_.GetWeakPtr()));
  std::unique_ptr<views::MenuItemView> root = menu_model_adapter_->CreateMenu();
  menu_runner_ = std::make_unique<views::MenuRunner>(
      std::move(root),
      views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU);
  menu_runner_->RunMenuAt(
      anchor_view->GetWidget(), nullptr, anchor_view->GetAnchorBoundsInScreen(),
      views::MenuAnchorPosition::kTopLeft, ui::mojom::MenuSourceType::kMouse);
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
    page_action_controller_->SetOverrideTriggerableEvent(
        kActionShowPartitionedStorage, std::nullopt);
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
  const auto truncated_name =
      gfx::TruncateString(name, 20, gfx::BreakType::CHARACTER_BREAK);
  page_action_controller_->OverrideText(kActionShowPartitionedStorage,
                                        truncated_name);

  page_action_controller_->OverrideAccessibleName(kActionShowPartitionedStorage,
                                                  name);
  page_action_controller_->OverrideTooltip(kActionShowPartitionedStorage, name);
  page_action_controller_->OverrideChipColors(
      kActionShowPartitionedStorage, model->background_color(), SK_ColorWHITE);
  page_action_controller_->SetOverrideHeight(kActionShowPartitionedStorage, 20);
  page_action_controller_->SetOverrideTriggerableEvent(
      kActionShowPartitionedStorage, ui::EF_RIGHT_MOUSE_BUTTON);
}

void PartitionedStoragePageActionController::OnPartitionedStorageMenuClosed() {
  CHECK_DEREF(action_item_for_menu_).SetIsShowingBubble(false);
  action_item_for_menu_ = nullptr;

  if (views::View* anchor = menu_anchor_view_tracker_.view()) {
    if (views::Button* button = views::Button::AsButton(anchor)) {
      button->SetHighlighted(false);
    }
  }
  menu_anchor_view_tracker_.SetView(nullptr);

  menu_runner_.reset();
  menu_model_adapter_.reset();
  containers_menu_model_.reset();
  containers_menu_delegate_.reset();
}

}  // namespace page_actions
