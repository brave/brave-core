/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_side_panel_header_controller.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/common/pref_names.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/layout_provider.h"

namespace {

constexpr int kHeaderTitleFontSize = 16;
constexpr int kHeaderButtonSize = 20;

std::unique_ptr<views::ImageButton> CreateHeaderImageButton(
    views::Button::PressedCallback callback,
    const gfx::VectorIcon& icon,
    int tooltip_id) {
  auto button = std::make_unique<views::ImageButton>(std::move(callback));
  button->SetTooltipText(l10n_util::GetStringUTF16(tooltip_id));
  button->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(icon, kColorSidebarPanelHeaderButton,
                                     kHeaderButtonSize));
  button->SetImageModel(
      views::Button::STATE_HOVERED,
      ui::ImageModel::FromVectorIcon(
          icon, kColorSidebarPanelHeaderButtonHovered, kHeaderButtonSize));
  return button;
}

// Per-panel header titles. We deliberately don't read from the entry's action
// item because that text differs from the label shown in the header.
std::u16string GetEntryTitle(SidePanelEntry::Id id) {
  int message_id = 0;
  switch (id) {
    case SidePanelEntry::Id::kReadingList:
      message_id = IDS_SIDEBAR_READING_LIST_PANEL_HEADER_TITLE;
      break;
    case SidePanelEntry::Id::kBookmarks:
      message_id = IDS_BOOKMARK_MANAGER_TITLE;
      break;
    default:
      return std::u16string();
  }
  return l10n_util::GetStringUTF16(message_id);
}

}  // namespace

BraveSidePanelHeaderController::BraveSidePanelHeaderController(
    BrowserWindowInterface& browser_window,
    SidePanelEntry* entry)
    : browser_window_(browser_window), entry_(entry->GetWeakPtr()) {
  pref_change_registrar_.Init(browser_window_->GetProfile()->GetPrefs());
  pref_change_registrar_.Add(
      kWebViewRoundedCorners,
      base::BindRepeating(
          &BraveSidePanelHeaderController::OnWebViewRoundedCornersPrefChanged,
          base::Unretained(this)));
}

BraveSidePanelHeaderController::~BraveSidePanelHeaderController() = default;

std::unique_ptr<views::Label>
BraveSidePanelHeaderController::CreatePanelTitle() {
  CHECK(entry_);
  auto label =
      std::make_unique<views::Label>(GetEntryTitle(entry_->key().id()));
  const int size_delta =
      kHeaderTitleFontSize - views::Label::GetDefaultFontList().GetFontSize();
  label->SetFontList(views::Label::GetDefaultFontList()
                         .DeriveWithSizeDelta(size_delta)
                         .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD));
  label->SetEnabledColor(kColorSidebarPanelHeaderTitle);
  label->SetAutoColorReadabilityEnabled(false);
  return label;
}

std::unique_ptr<views::ImageButton>
BraveSidePanelHeaderController::CreateLaunchButton() {
  CHECK(entry_);
  if (entry_->key().id() == SidePanelEntry::Id::kBookmarks) {
    return CreateHeaderImageButton(
        base::BindRepeating(
            &BraveSidePanelHeaderController::OnLaunchButtonPressed,
            weak_factory_.GetWeakPtr(), GURL(chrome::kChromeUIBookmarksURL)),
        kLeoLaunchIcon,
        IDS_SIDEBAR_READING_LIST_PANEL_HEADER_BOOKMARKS_BUTTON_TOOLTIP);
  }

  return nullptr;
}

std::unique_ptr<views::ImageButton>
BraveSidePanelHeaderController::CreateCloseButton() {
  return CreateHeaderImageButton(
      base::BindRepeating(&BraveSidePanelHeaderController::OnCloseButtonPressed,
                          weak_factory_.GetWeakPtr()),
      kLeoCloseIcon, IDS_SIDEBAR_PANEL_CLOSE_BUTTON_TOOLTIP);
}

int BraveSidePanelHeaderController::GetTopRadius() const {
  if (!browser_window_->GetProfile()->GetPrefs()->GetBoolean(
          kWebViewRoundedCorners)) {
    return 0;
  }
  return views::LayoutProvider::Get()->GetDistanceMetric(
      ChromeDistanceMetric::DISTANCE_SIDE_PANEL_CONTENT_RADIUS);
}

void BraveSidePanelHeaderController::SetUpdateHeaderCallback(
    base::RepeatingClosure callback) {
  update_header_callback_ = std::move(callback);
}

void BraveSidePanelHeaderController::OnLaunchButtonPressed(const GURL& url) {
  ShowSingletonTab(base::to_address(browser_window_), url);
}

void BraveSidePanelHeaderController::OnCloseButtonPressed() {
  if (auto* side_panel_ui = browser_window_->GetFeatures().side_panel_ui()) {
    side_panel_ui->Close();
  }
}

void BraveSidePanelHeaderController::OnWebViewRoundedCornersPrefChanged() {
  if (update_header_callback_) {
    update_header_callback_.Run();
  }
}
