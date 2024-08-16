/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_add_bubble_view.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_runner.h"
#include "base/time/time.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/browser/ui/views/playlist/playlist_bubbles_controller.h"
#include "brave/browser/ui/views/playlist/playlist_edit_bubble_view.h"
#include "brave/browser/ui/views/playlist/thumbnail_provider.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "chrome/grit/generated_resources.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/ui_base_types.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/progress_ring_utils.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/window/dialog_delegate.h"

namespace gfx {
class Canvas;
}

namespace playlist {
namespace {
// LoadingSpinner represents the loading animation for the 'Add bubble'
class LoadingSpinner : public views::View, public gfx::AnimationDelegate {
  METADATA_HEADER(LoadingSpinner, views::View)
 public:
  LoadingSpinner() {
    animation_.SetSlideDuration(base::Milliseconds(2500));
    animation_.SetTweenType(gfx::Tween::LINEAR);

    constexpr int kSpinnerSize = 40;
    SetPreferredSize(gfx::Size(kSpinnerSize, kSpinnerSize));
  }

  ~LoadingSpinner() override = default;

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override {
    if (!animation_.is_animating()) {
      animation_.Show();
    }

    constexpr int kSpinnerStrokeWidth = 4;
    auto preferred_size = GetPreferredSize();
    preferred_size.Enlarge(-kSpinnerStrokeWidth, -kSpinnerStrokeWidth);
    auto origin =
        GetLocalBounds().CenterPoint() -
        gfx::Vector2d(preferred_size.width() / 2, preferred_size.height() / 2);

    SkColor foreground_color = SkColorSetRGB(0x3f, 0x39, 0xe8);
    SkColor background_color = SkColorSetA(foreground_color, 0.3 * 255);
    views::DrawSpinningRing(
        canvas, gfx::RectToSkRect(gfx::Rect(origin, preferred_size)),
        background_color, foreground_color, kSpinnerStrokeWidth,
        gfx::Tween::IntValueBetween(animation_.GetCurrentValue(), 0, 360));
  }

  // gfx::AnimationDelegate:
  void AnimationProgressed(const gfx::Animation* animation) override {
    SchedulePaint();
  }

  void AnimationEnded(const gfx::Animation* animation) override {
    animation_.Reset();
    animation_.Show();
  }

 private:
  gfx::SlideAnimation animation_{this};
};

BEGIN_METADATA(LoadingSpinner)
END_METADATA
}  // namespace

PlaylistAddBubbleView::PlaylistAddBubbleView(
    views::View* anchor_view,
    base::WeakPtr<PlaylistTabHelper> tab_helper)
    : PlaylistBubbleView(anchor_view, std::move(tab_helper)),
      thumbnail_provider_(
          std::make_unique<ThumbnailProvider>(tab_helper_.get())) {
  tab_helper_observation_.Observe(tab_helper_.get());
  // What this looks like:
  // https://user-images.githubusercontent.com/5474642/243532255-f82fc740-eea0-4c52-b43a-378ab703d229.png
  SetTitle(l10n_util::GetStringUTF16(IDS_PLAYLIST_ADD_TO_PLAYLIST));

  SetLayoutManager(std::make_unique<views::BoxLayout>(
                       views::BoxLayout::Orientation::kVertical, gfx::Insets(),
                       /*between_child_spacing=*/8))
      ->set_cross_axis_alignment(
          views::BoxLayout::CrossAxisAlignment::kStretch);

  auto* header = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_PLAYLIST_MEDIA_FOUND_IN_THIS_PAGE)));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view_->ClipHeightTo(/*min_height=*/0, /*max_height=*/230);
  scroll_view_->SetDrawOverflowIndicator(false);
  scroll_view_->SetBorder(views::CreateThemedRoundedRectBorder(
      /*thickness=*/1,
      /*corner_radius=*/4.f, kColorBravePlaylistListBorder));
  scroll_view_->SetVisible(false);
  scroll_view_->SetContents(std::make_unique<views::View>());
  // Fix preferred width. This is for ignoring insets that could be added by
  // border.
  scroll_view_->SetPreferredSize(
      gfx::Size(kWidth, scroll_view_->GetPreferredSize().height()));

  loading_spinner_ = AddChildView(std::make_unique<LoadingSpinner>());

  SetButtonLabel(ui::DialogButton::DIALOG_BUTTON_OK,
                 l10n_util::GetStringUTF16(IDS_PLAYLIST_ADD_SELECTED));
  SetButtonEnabled(ui::DIALOG_BUTTON_OK, false);
  SetButtonEnabled(ui::DIALOG_BUTTON_CANCEL, false);

  if (!tab_helper_->is_adding_items()) {
    InitListView();
  }
}

PlaylistAddBubbleView::~PlaylistAddBubbleView() = default;

void PlaylistAddBubbleView::PlaylistTabHelperWillBeDestroyed() {
  tab_helper_observation_.Reset();
}

void PlaylistAddBubbleView::OnAddedItemFromTabHelper(
    const std::vector<mojom::PlaylistItemPtr>& items) {
  if (items.empty()) {
    AddChildView(std::make_unique<views::Label>(
        l10n_util::GetStringUTF16(IDS_PLAYLIST_MEDIA_NOT_FOUND_IN_THIS_PAGE)));
    loading_spinner_->SetVisible(false);
    SetButtonEnabled(ui::DIALOG_BUTTON_CANCEL, true);
    return SizeToContents();
  }

  next_bubble_ = PlaylistBubblesController::BubbleType::kEdit;
  GetWidget()->Close();
}

void PlaylistAddBubbleView::InitListView() {
  CHECK(scroll_view_);
  CHECK(!list_view_);
  loading_spinner_->SetVisible(false);
  scroll_view_->SetVisible(true);
  list_view_ = scroll_view_->SetContents(std::make_unique<SelectableItemsView>(
      thumbnail_provider_.get(), tab_helper_->found_items(),
      base::BindRepeating(&PlaylistAddBubbleView::OnSelectionChanged,
                          base::Unretained(this))));
  list_view_->SetSelected(tab_helper_->found_items());

  SetAcceptCallbackWithClose(base::BindRepeating(
      &PlaylistAddBubbleView::AddSelected, base::Unretained(this)));
  SetButtonEnabled(ui::DIALOG_BUTTON_OK, true);
  SetButtonEnabled(ui::DIALOG_BUTTON_CANCEL, true);

  scroll_view_->SetPreferredSize(std::nullopt);
  scroll_view_->SetPreferredSize(
      gfx::Size(kWidth, scroll_view_->GetPreferredSize().height()));
  if (GetWidget()) {
    SizeToContents();
  }
}

bool PlaylistAddBubbleView::AddSelected() {
  if (!tab_helper_ || tab_helper_->is_adding_items()) {
    return true;
  }

  SetButtonEnabled(ui::DIALOG_BUTTON_OK, false);
  SetButtonEnabled(ui::DIALOG_BUTTON_CANCEL, false);
  scroll_view_->SetVisible(false);
  loading_spinner_->SetVisible(true);
  SizeToContents();

  std::vector<mojom::PlaylistItemPtr> items = list_view_->GetSelected();
  CHECK(items.size())
      << "The button should be disabled when nothing is selected.";

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&PlaylistTabHelper::AddItems, tab_helper_,
                                std::move(items)));

  return false;
}

void PlaylistAddBubbleView::OnSelectionChanged() {
  if (bool has_selected = list_view_->HasSelected();
      has_selected != IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK)) {
    SetButtonEnabled(ui::DIALOG_BUTTON_OK, has_selected);
  }
}

BEGIN_METADATA(PlaylistAddBubbleView)
END_METADATA
}  // namespace playlist
