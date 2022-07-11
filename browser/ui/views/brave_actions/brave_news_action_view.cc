// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_actions/brave_news_action_view.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/callback_helpers.h"
#include "base/task/task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/browser/ui/views/brave_actions/brave_action_view.h"
#include "brave/browser/ui/views/brave_news/brave_news_bubble_view.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/browser_context.h"
#include "extensions/common/constants.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_rep_default.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace {
SkColor selectedColor = SkColorSetRGB(30, 33, 82);
}

BraveTodayActionView::BraveTodayActionView(Profile* profile,
                                           TabStripModel* tab_strip)
    : views::LabelButton(base::BindRepeating(&BraveTodayActionView::ShowBubble,
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

  should_show_.Init(brave_news::prefs::kShouldShowToolbarButton,
                    profile->GetPrefs(),
                    base::BindRepeating(&BraveTodayActionView::Update,
                                        base::Unretained(this)));

  Update();
}

BraveTodayActionView::~BraveTodayActionView() {
  tab_strip_->RemoveObserver(this);
}

void BraveTodayActionView::Init() {
  Update();
}

void BraveTodayActionView::Update() {
  if (!should_show_.GetValue()) {
    SetVisible(false);
    return;
  }

  auto* contents = tab_strip_->GetActiveWebContents();
  bool subscribed = false;
  absl::optional<BraveNewsTabHelper::FeedDetails> feed = absl::nullopt;

  if (contents) {
    auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents);
    if (!tab_helper->available_feeds().empty()) {
      feed = tab_helper->available_feeds()[0];
      subscribed = tab_helper->is_subscribed();
    }
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
  SetVisible(!!feed);
}

SkPath BraveTodayActionView::GetHighlightPath() const {
  auto highlight_insets = gfx::Insets();
  gfx::Rect rect(GetPreferredSize());
  rect.Inset(highlight_insets);
  const int radii = ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kMaximum, rect.size());
  SkPath path;
  path.addRoundRect(gfx::RectToSkRect(rect), radii, radii);
  return path;
}

std::u16string BraveTodayActionView::GetTooltipText(const gfx::Point& p) const {
  return l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_ACTION_VIEW_TOOLTIP);
}

std::unique_ptr<views::LabelButtonBorder>
BraveTodayActionView::CreateDefaultBorder() const {
  std::unique_ptr<views::LabelButtonBorder> border =
      LabelButton::CreateDefaultBorder();
  border->set_insets(gfx::Insets::TLBR(3, 0, 3, 0));
  return border;
}

void BraveTodayActionView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.old_contents != selection.new_contents) {
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

void BraveTodayActionView::OnAvailableFeedsChanged(
    const std::vector<BraveNewsTabHelper::FeedDetails>& feeds) {
  Update();
}

void BraveTodayActionView::ShowBubble() {
  if (!tab_strip_->GetActiveWebContents())
    return;

  auto* tab_helper =
      BraveNewsTabHelper::FromWebContents(tab_strip_->GetActiveWebContents());
  if (tab_helper->available_feeds().empty())
    return;

  if (bubble_widget_) {
    bubble_widget_->Close();
  }

  bubble_widget_ =
      BraveNewsBubbleView::Show(this, tab_strip_->GetActiveWebContents());
}
