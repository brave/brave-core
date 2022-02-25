/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"

#include <memory>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/one_shot_event.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/brave_actions/brave_action_view_controller.h"
#include "brave/browser/ui/views/brave_actions/brave_action_view.h"
#include "brave/browser/ui/views/brave_actions/brave_rewards_action_stub_view.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"
#include "brave/browser/ui/views/rounded_separator.h"
#include "brave/common/brave_switches.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_shields/common/features.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar_bubble_delegate.h"
#include "chrome/browser/ui/views/toolbar/toolbar_action_view.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_action_manager.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/view.h"

BraveActionsContainer::BraveActionInfo::BraveActionInfo()
    : position_(ACTION_ANY_POSITION) {}

BraveActionsContainer::BraveActionInfo::~BraveActionInfo() {
  Reset();
}

void BraveActionsContainer::BraveActionInfo::Reset() {
  // Destroy view before view_controller.
  // Destructor for |ToolbarActionView| tries to access controller instance.
  view_.reset();
  view_controller_.reset();
}

BraveActionsContainer::BraveActionsContainer(Browser* browser, Profile* profile)
    : views::View(),
      extensions::BraveActionAPI::Observer(),
      browser_(browser),
      extension_system_(extensions::ExtensionSystem::Get(profile)),
      extension_action_api_(extensions::ExtensionActionAPI::Get(profile)),
      extension_registry_(extensions::ExtensionRegistry::Get(profile)),
      extension_action_manager_(
          extensions::ExtensionActionManager::Get(profile)),
      brave_action_api_(extensions::BraveActionAPI::Get(browser)),
      rewards_service_(
          brave_rewards::RewardsServiceFactory::GetForProfile(profile)),
      weak_ptr_factory_(this) {
  // Handle when the extension system is ready
  extension_system_->ready().Post(
      FROM_HERE, base::BindOnce(&BraveActionsContainer::OnExtensionSystemReady,
                                weak_ptr_factory_.GetWeakPtr()));
}

BraveActionsContainer::~BraveActionsContainer() {
  actions_.clear();
}

void BraveActionsContainer::Init() {
  // automatic layout
  auto vertical_container_layout = std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal);
  vertical_container_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  vertical_container_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  SetLayoutManager(std::move(vertical_container_layout));

  // children
  RoundedSeparator* brave_button_separator_ = new RoundedSeparator();
  // TODO(petemill): theme color
  brave_button_separator_->SetColor(SkColorSetRGB(0xb2, 0xb5, 0xb7));
  constexpr int kSeparatorMargin = 3;
  constexpr int kSeparatorWidth = 1;
  brave_button_separator_->SetPreferredSize(gfx::Size(
                                    kSeparatorWidth + kSeparatorMargin*2,
                                    GetLayoutConstant(LOCATION_BAR_ICON_SIZE)));
  // separator left & right margin
  brave_button_separator_->SetBorder(
      views::CreateEmptyBorder(0, kSeparatorMargin, 0, kSeparatorMargin));
  // Just in case the extensions load before this function does (not likely!)
  // make sure separator is at index 0
  AddChildViewAt(brave_button_separator_, 0);
  AddActionViewForShields();
  // Populate actions
  actions_[brave_extension_id].position_ = 1;
  actions_[brave_rewards_extension_id].position_ = ACTION_ANY_POSITION;

  // React to Brave Rewards preferences changes.
  show_brave_rewards_button_.Init(
      brave_rewards::prefs::kShowButton, browser_->profile()->GetPrefs(),
      base::BindRepeating(
          &BraveActionsContainer::OnBraveRewardsPreferencesChanged,
          base::Unretained(this)));
}

bool BraveActionsContainer::IsContainerAction(const std::string& id) const {
  return (actions_.find(id) != actions_.end());
}

bool BraveActionsContainer::ShouldShowAction(const std::string& id) const {
  if (!IsContainerAction(id))
    return false;
  if (popup_owner_ && actions_.at(id).view_controller_.get() == popup_owner_)
    return true;
  if (id == brave_rewards_extension_id)
    return ShouldShowBraveRewardsAction();
  return true;
}

bool BraveActionsContainer::ShouldShowBraveRewardsAction() const {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kDisableBraveRewardsExtension)) {
    return false;
  }

  if (!brave::IsRegularProfile(browser_->profile())) {
    return false;
  }

  const PrefService* prefs = browser_->profile()->GetPrefs();
  return prefs->GetBoolean(brave_rewards::prefs::kShowButton);
}

void BraveActionsContainer::AddAction(const extensions::Extension* extension) {
  DCHECK(extension);

  const std::string& id = extension->id();
  if (!IsContainerAction(id))
    return;

  VLOG(1) << "AddAction (" << id
          << "), was already loaded: " << static_cast<bool>(actions_[id].view_);

  if (!actions_[id].view_controller_) {
    // Remove existing stub view, if present
    actions_[id].Reset();
    // Create a ExtensionActionViewController for the extension
    actions_[id].view_controller_ =
        BraveActionViewController::Create(id, browser_, this);
    // The button view
    actions_[id].view_ = std::make_unique<BraveActionView>(
        actions_[id].view_controller_.get(), this);
    AttachAction(id);
    // Handle if we are in a continuing pressed state for this extension.
    if (is_rewards_pressed_ && id == brave_rewards_extension_id) {
      is_rewards_pressed_ = false;
      actions_[id].view_controller_->ExecuteUserAction(
          ToolbarActionViewController::InvocationSource::kToolbarButton);
    }
  }
}

void BraveActionsContainer::AddActionStubForRewards() {
  const std::string id = brave_rewards_extension_id;
  if (actions_[id].view_) {
    return;
  }
  actions_[id].view_ = std::make_unique<BraveRewardsActionStubView>(
      browser_->profile(), this);
  AttachAction(id);
}

void BraveActionsContainer::AttachAction(const std::string& id) {
  DCHECK(IsContainerAction(id));
  DCHECK(actions_[id].view_);

  const auto& action = actions_[id];

  if (!ShouldShowAction(id))
    action.view_->SetVisible(false);

  // Add extension view after separator view
  // `AddChildView` should be called first, so that changes that modify
  // layout (e.g. preferred size) are forwarded to its parent
  if (action.position_ != ACTION_ANY_POSITION) {
    DCHECK_GT(action.position_, 0);
    AddChildViewAt(action.view_.get(), action.position_);
  } else {
    AddChildView(action.view_.get());
  }
  // we control destruction
  action.view_->set_owned_by_client();
  Update();
  PreferredSizeChanged();
}

void BraveActionsContainer::AddAction(const std::string& id) {
  DCHECK(extension_registry_);
  const extensions::Extension* extension =
      extension_registry_->enabled_extensions().GetByID(id);
  if (extension) {
    AddAction(extension);
    return;
  }
  if (id == brave_rewards_extension_id) {
    AddActionStubForRewards();
    return;
  }
  LOG(ERROR) << "Extension not found for Brave Action: " << id;
}

void BraveActionsContainer::RemoveAction(const std::string& id) {
  DCHECK(IsContainerAction(id));
  VLOG(1) << "RemoveAction (" << id << "), was loaded: "
          << static_cast<bool>(actions_[id].view_);
  // This will reset references and automatically remove the child from the
  // parent (us)
  actions_[id].Reset();
  // layout
  Update();
  PreferredSizeChanged();
}

void BraveActionsContainer::UpdateActionVisibility(const std::string& id) {
  if (views::Button* button = GetActionButton(id)) {
    bool should_show = ShouldShowAction(id);
    if (button->GetVisible() != should_show) {
      button->SetVisible(should_show);
      Update();
    }
  }
}

views::Button* BraveActionsContainer::GetActionButton(
    const std::string& id) const {
  return IsContainerAction(id) ? actions_.at(id).view_.get() : nullptr;
}

bool BraveActionsContainer::IsActionShown(const std::string& id) const {
  views::Button* button = GetActionButton(id);
  return button && button->GetVisible();
}

void BraveActionsContainer::UpdateActionState(const std::string& id) {
  if (actions_[id].view_controller_)
    actions_[id].view_controller_->UpdateState();
}

void BraveActionsContainer::AddActionViewForShields() {
  if (base::FeatureList::IsEnabled(
          brave_shields::features::kBraveShieldsPanelV2)) {
    shields_action_btn_ =
        AddChildViewAt(std::make_unique<BraveShieldsActionView>(
                           browser_->profile(), browser_->tab_strip_model()),
                       1);
    shields_action_btn_->SetPreferredSize(GetToolbarActionSize());
    shields_action_btn_->Init();
  }
}

void BraveActionsContainer::Update() {
  // Update state of each action and also determine if there are any buttons to
  // show
  if (shields_action_btn_) {
    shields_action_btn_->Update();
  }

  bool can_show = false;
  for (auto const& pair : actions_) {
    if (pair.second.view_controller_)
      pair.second.view_controller_->UpdateState();
    if (!can_show && pair.second.view_ && pair.second.view_->GetVisible())
      can_show = true;
  }
  // only show separator if we're showing any buttons
  const bool visible = !should_hide_ && can_show;
  SetVisible(visible);
  Layout();
}

void BraveActionsContainer::SetShouldHide(bool should_hide) {
  should_hide_ = should_hide;
  Update();
}

content::WebContents* BraveActionsContainer::GetCurrentWebContents() {
  return browser_->tab_strip_model()->GetActiveWebContents();
}

views::LabelButton* BraveActionsContainer::GetOverflowReferenceView() const {
  // Our action views should always be visible,
  // so we should not need another view.
  NOTREACHED();
  return nullptr;
}

// ToolbarActionView::Delegate members
gfx::Size BraveActionsContainer::GetToolbarActionSize() {
  // Width > Height to give space for a large bubble (especially for shields).
  // TODO(petemill): Generate based on toolbar size.
  return gfx::Size(34, 24);
}

void BraveActionsContainer::WriteDragDataForView(View* sender,
                                                   const gfx::Point& press_pt,
                                                   OSExchangeData* data) {
  // Not supporting drag for action buttons inside this container
}

int BraveActionsContainer::GetDragOperationsForView(View* sender,
                                                      const gfx::Point& p) {
  return ui::DragDropTypes::DRAG_NONE;
}

bool BraveActionsContainer::CanStartDragForView(View* sender,
                                                  const gfx::Point& press_pt,
                                                  const gfx::Point& p) {
  return false;
}
// end ToolbarActionView::Delegate members

// BraveRewardsActionStubView::Delegate members
void BraveActionsContainer::OnRewardsStubButtonClicked() {
  // Keep button state visually pressed until new extension button
  // takes over.
  actions_[brave_rewards_extension_id].view_->SetState(
      views::Button::STATE_PRESSED);
  extensions::ExtensionService* service =
           extension_system_->extension_service();
  if (service) {
    is_rewards_pressed_ = true;
    extensions::ComponentLoader* loader = service->component_loader();
          static_cast<extensions::BraveComponentLoader*>(loader)->
              AddRewardsExtension();

    if (rewards_service_) {
      rewards_service_->StartProcess(base::DoNothing());
    }
  }
}
// end BraveRewardsActionStubView::Delegate members

void BraveActionsContainer::OnExtensionSystemReady() {
  // observe changes in extension system
  extension_registry_observer_.Observe(extension_registry_);
  extension_action_observer_.Observe(extension_action_api_);
  brave_action_observer_.Observe(brave_action_api_);
  // Check if extensions already loaded
  AddAction(brave_extension_id);
  AddAction(brave_rewards_extension_id);
}

// ExtensionRegistry::Observer
void BraveActionsContainer::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (IsContainerAction(extension->id()))
    AddAction(extension);
}

void BraveActionsContainer::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  if (IsContainerAction(extension->id()))
    RemoveAction(extension->id());
}
// end ExtensionRegistry::Observer

// ExtensionActionAPI::Observer
void BraveActionsContainer::OnExtensionActionUpdated(
    extensions::ExtensionAction* extension_action,
    content::WebContents* web_contents,
    content::BrowserContext* browser_context) {
  if (IsContainerAction(extension_action->extension_id()))
    UpdateActionState(extension_action->extension_id());
}
// end ExtensionActionAPI::Observer

// BraveActionAPI::Observer
void BraveActionsContainer::OnBraveActionShouldTrigger(
    const std::string& extension_id,
    std::unique_ptr<std::string> ui_relative_path) {
  if (!IsContainerAction(extension_id)) {
    return;
  }
  if (actions_[extension_id].view_controller_) {
    if (ui_relative_path)
      actions_[extension_id].view_controller_
          ->ExecuteActionUI(*ui_relative_path);
    else
      actions_[extension_id].view_controller_->ExecuteUserAction(
          ToolbarActionViewController::InvocationSource::kApi);
  }
}

void BraveActionsContainer::ChildPreferredSizeChanged(views::View* child) {
  PreferredSizeChanged();
}

// Brave Rewards preferences change observers callback
void BraveActionsContainer::OnBraveRewardsPreferencesChanged() {
  UpdateActionVisibility(brave_rewards_extension_id);
}

ToolbarActionViewController* BraveActionsContainer::GetActionForId(
    const std::string& action_id) {
  return nullptr;
}

ToolbarActionViewController* BraveActionsContainer::GetPoppedOutAction() const {
  return nullptr;
}

void BraveActionsContainer::OnContextMenuShown(
    ToolbarActionViewController* extension) {}

void BraveActionsContainer::OnContextMenuClosed(
    ToolbarActionViewController* extension) {}

bool BraveActionsContainer::IsActionVisibleOnToolbar(
    const ToolbarActionViewController* action) const {
  return false;
}

extensions::ExtensionContextMenuModel::ButtonVisibility
BraveActionsContainer::GetActionVisibility(
    const ToolbarActionViewController* action) const {
  return extensions::ExtensionContextMenuModel::PINNED;
}

void BraveActionsContainer::UndoPopOut() {}

void BraveActionsContainer::SetPopupOwner(
    ToolbarActionViewController* popup_owner) {
  if (popup_owner) {
    DCHECK(!popup_owner_);
    popup_owner_ = popup_owner;
    UpdateActionVisibility(popup_owner->GetId());
  } else if (popup_owner_) {
    auto* previous_owner = popup_owner_;
    popup_owner_ = nullptr;
    UpdateActionVisibility(previous_owner->GetId());
  }
}

void BraveActionsContainer::HideActivePopup() {
  if (popup_owner_)
    popup_owner_->HidePopup();
}

bool BraveActionsContainer::CloseOverflowMenuIfOpen() {
  return false;
}

void BraveActionsContainer::PopOutAction(ToolbarActionViewController* action,
                                         base::OnceClosure closure) {}

bool BraveActionsContainer::ShowToolbarActionPopupForAPICall(
    const std::string& action_id,
    ShowPopupCallback callback) {
  return false;
}

void BraveActionsContainer::ShowToolbarActionBubble(
    std::unique_ptr<ToolbarActionsBarBubbleDelegate> bubble) {}

void BraveActionsContainer::ShowToolbarActionBubbleAsync(
    std::unique_ptr<ToolbarActionsBarBubbleDelegate> bubble) {}

void BraveActionsContainer::ToggleExtensionsMenu() {}

bool BraveActionsContainer::HasAnyExtensions() const {
  return false;
}
