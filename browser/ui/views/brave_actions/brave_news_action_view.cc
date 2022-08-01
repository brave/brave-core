// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_actions/brave_news_action_view.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/browser/ui/views/brave_news/brave_news_bubble_view.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/grit/brave_components_strings.h"
#include "extensions/common/constants.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/controls/button/menu_button_controller.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace {
constexpr SkColor selectedColor = SkColorSetRGB(30, 33, 82);
}

BraveNewsActionView::BraveNewsActionView(Profile* profile,
                                         TabStripModel* tab_strip)
    : views::LabelButton(
          base::BindRepeating(&BraveNewsActionView::ButtonPressed,
                              base::Unretained(this))),
      profile_(profile),
      tab_strip_(tab_strip) {
  DCHECK(profile_);
  SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_ACTION_VIEW_TOOLTIP));
  SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_CENTER);

  auto* ink_drop = views::InkDrop::Get(this);
  ink_drop->SetMode(views::InkDropHost::InkDropMode::ON);
  ink_drop->SetBaseColorCallback(base::BindRepeating(
      [](views::View* host) { return GetToolbarInkDropBaseColor(host); },
      this));
  ink_drop->SetVisibleOpacity(kToolbarInkDropVisibleOpacity);
  SetHasInkDropActionOnClick(true);

  tab_strip_->AddObserver(this);
  if (tab_strip_->GetActiveWebContents()) {
    BraveNewsTabHelper::FromWebContents(tab_strip_->GetActiveWebContents())
        ->AddObserver(this);
  }

  should_show_.Init(brave_news::prefs::kShouldShowToolbarButton,
                    profile->GetPrefs(),
                    base::BindRepeating(&BraveNewsActionView::Update,
                                        base::Unretained(this)));

  auto menu_button_controller = std::make_unique<views::MenuButtonController>(
      this,
      base::BindRepeating(&BraveNewsActionView::ButtonPressed,
                          base::Unretained(this)),
      std::make_unique<views::Button::DefaultButtonControllerDelegate>(this));
  SetButtonController(std::move(menu_button_controller));

  Update();
}

BraveNewsActionView::~BraveNewsActionView() {
  tab_strip_->RemoveObserver(this);
  if (tab_strip_->GetActiveWebContents()) {
    BraveNewsTabHelper::FromWebContents(tab_strip_->GetActiveWebContents())
        ->RemoveObserver(this);
  }
}

void BraveNewsActionView::Init() {
  Update();
}

void BraveNewsActionView::Update() {
  if (!should_show_.GetValue()) {
    SetVisible(false);
    return;
  }

  auto* contents = tab_strip_->GetActiveWebContents();
  bool subscribed = false;
  bool has_feeds = false;
  absl::optional<BraveNewsTabHelper::FeedDetails> feed;

  if (contents) {
    auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents);
    subscribed = tab_helper->IsSubscribed();
    has_feeds = !tab_helper->GetAvailableFeeds().empty();
  }

  auto background =
      subscribed ? views::CreateRoundedRectBackground(
                       selectedColor,
                       ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
                           views::Emphasis::kMaximum, GetPreferredSize()))
                 : nullptr;
  auto image =
      gfx::CreateVectorIcon(kBraveTodaySubscribeIcon, 16,
                            color_utils::DeriveDefaultIconColor(
                                subscribed ? SK_ColorWHITE : SK_ColorBLACK));
  SetImage(ButtonState::STATE_NORMAL, image);
  SetBackground(std::move(background));
  SetVisible(has_feeds);
}

SkPath BraveNewsActionView::GetHighlightPath() const {
  gfx::Rect rect(GetPreferredSize());
  const int radii = ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kMaximum, rect.size());
  SkPath path;
  path.addRoundRect(gfx::RectToSkRect(rect), radii, radii);
  return path;
}

std::u16string BraveNewsActionView::GetTooltipText(const gfx::Point& p) const {
  return l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_ACTION_VIEW_TOOLTIP);
}

std::unique_ptr<views::LabelButtonBorder>
BraveNewsActionView::CreateDefaultBorder() const {
  std::unique_ptr<views::LabelButtonBorder> border =
      LabelButton::CreateDefaultBorder();
  border->set_insets(gfx::Insets::VH(3, 0));
  return border;
}

void BraveNewsActionView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    if (selection.old_contents) {
      BraveNewsTabHelper::FromWebContents(selection.old_contents)
          ->RemoveObserver(this);
    }

    if (selection.new_contents) {
      BraveNewsTabHelper::FromWebContents(selection.new_contents)
          ->AddObserver(this);
    }
  }

  Update();
}

void BraveNewsActionView::OnAvailableFeedsChanged(
    const std::vector<BraveNewsTabHelper::FeedDetails>& feeds) {
  Update();
}

void BraveNewsActionView::ButtonPressed() {
  // If the bubble is already open, do nothing.
  if (bubble_widget_) {
    return;
  }

  bubble_widget_ =
      BraveNewsBubbleView::Show(this, tab_strip_->GetActiveWebContents());
}
