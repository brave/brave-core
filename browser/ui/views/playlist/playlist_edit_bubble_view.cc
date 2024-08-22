/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_edit_bubble_view.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/ranges/algorithm.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/playlist/playlist_action_dialogs.h"
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/browser/ui/views/playlist/playlist_add_bubble_view.h"
#include "brave/browser/ui/views/playlist/playlist_bubbles_controller.h"
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/base/ui_base_types.h"
#include "ui/color/color_id.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_delegate.h"

namespace playlist {
namespace {
class Row : public views::LabelButton {
  METADATA_HEADER(Row, views::LabelButton)
 public:
  Row(const std::u16string& text,
      const ui::ImageModel& icon,
      views::Button::PressedCallback callback = {})
      : LabelButton(std::move(callback), text) {
    SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_RIGHT);
    SetImageModel(views::Button::STATE_NORMAL, icon);
    label()->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);
  }

  ~Row() override = default;

  // views::LabelButton:
  void Layout(PassKey) override {
    LayoutSuperclass<LabelButton>(this);
    // Extend |label|'s width so the this button's sub controls are justified.
    const auto contents_x = GetContentsBounds().x();
    label()->SetX(contents_x);
    label()->SetSize(
        {label()->width() + label()->x() - contents_x, label()->height()});
  }
};

BEGIN_METADATA(Row)
END_METADATA
}  // namespace

PlaylistEditBubbleView::PlaylistEditBubbleView(
    views::View* anchor_view,
    base::WeakPtr<PlaylistTabHelper> tab_helper)
    : PlaylistBubbleView(anchor_view, std::move(tab_helper)) {
  browser_ = chrome::FindBrowserWithTab(&tab_helper_->GetWebContents());
  CHECK(browser_);

  // What this looks like:
  // https://user-images.githubusercontent.com/5474642/243532057-4bbbe779-47a1-4c3a-bd34-ce1334cf1d1d.png
  set_margins({});
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
                       views::BoxLayout::Orientation::kVertical,
                       gfx::Insets::VH(4, 16),
                       /*between_child_spacing =*/4))
      ->set_cross_axis_alignment(
          views::BoxLayout::CrossAxisAlignment::kStretch);
  ResetChildViews();

  tab_helper_observation_.Observe(tab_helper_.get());
}

PlaylistEditBubbleView::~PlaylistEditBubbleView() = default;

void PlaylistEditBubbleView::PlaylistTabHelperWillBeDestroyed() {
  tab_helper_observation_.Reset();
}

void PlaylistEditBubbleView::OnSavedItemsChanged(
    const std::vector<mojom::PlaylistItemPtr>& items) {
  if (auto* widget = GetWidget(); !widget || widget->IsClosed()) {
    return;
  }

  ResetChildViews();
  SizeToContents();
}

void PlaylistEditBubbleView::ResetChildViews() {
  RemoveAllChildViews();

  constexpr int kIconSize = 16;
  AddChildView(std::make_unique<Row>(
      l10n_util::GetStringUTF16(IDS_PLAYLIST_ADDED_TO_PLAYLIST),
      ui::ImageModel::FromVectorIcon(kLeoCheckCircleFilledIcon,
                                     kColorBravePlaylistAddedIcon, kIconSize)));
  bool added_separator = false;
  auto add_separator_if_needed = [&added_separator, this] {
    if (added_separator) {
      return;
    }

    added_separator = !!AddChildView(std::make_unique<views::Separator>());
  };

  const auto& saved_items = tab_helper_->saved_items();

  if (!saved_items.empty() && saved_items.front()->parents.size()) {
    add_separator_if_needed();
    AddChildView(std::make_unique<Row>(
        l10n_util::GetStringUTF16(IDS_PLAYLIST_OPEN_IN_PLAYLIST),
        ui::ImageModel::FromVectorIcon(kLeoProductPlaylistIcon,
                                       ui::kColorMenuIcon, kIconSize),
        base::BindRepeating(&PlaylistEditBubbleView::OpenInPlaylist,
                            base::Unretained(this))));
  }

  if (PlaylistMoveDialog::CanMoveItems(saved_items)) {
    add_separator_if_needed();
    AddChildView(std::make_unique<Row>(
        l10n_util::GetStringUTF16(IDS_PLAYLIST_CHANGE_FOLDER),
        ui::ImageModel::FromVectorIcon(kLeoFolderExchangeIcon,
                                       ui::kColorMenuIcon, kIconSize),
        base::BindRepeating(&PlaylistEditBubbleView::ChangeFolder,
                            base::Unretained(this))));
  }

  if (base::ranges::none_of(saved_items, [](const auto& item) {
        return item->parents.empty();
      })) {
    add_separator_if_needed();
    AddChildView(std::make_unique<Row>(
        l10n_util::GetStringUTF16(IDS_PLAYLIST_REMOVE_FROM_PLAYLIST),
        ui::ImageModel::FromVectorIcon(kLeoTrashIcon, ui::kColorMenuIcon,
                                       kIconSize),
        base::BindRepeating(&PlaylistEditBubbleView::RemoveFromPlaylist,
                            base::Unretained(this))));
  }

  if (tab_helper_->GetUnsavedItems().size()) {
    AddChildView(std::make_unique<views::Separator>());
    AddChildView(std::make_unique<Row>(
        l10n_util::GetStringUTF16(IDS_PLAYLIST_MORE_MEDIA_IN_THIS_PAGE),
        ui::ImageModel::FromVectorIcon(kLeoProductPlaylistIcon,
                                       ui::kColorMenuIcon, kIconSize),
        base::BindRepeating(&PlaylistEditBubbleView::MoreMediaInContents,
                            base::Unretained(this))));
  }
}

void PlaylistEditBubbleView::OpenInPlaylist() {
  // Technically, the saved items could belong to multiple playlists
  // at the same time and their parent playlists could be different from each
  // other's. But for simplicity, we just open the first one assuming that most
  // users keep items from a site in a same playlist.
  const auto& saved_items = tab_helper_->saved_items();
  CHECK(saved_items.size());
  CHECK(saved_items.front()->parents.size());
  const std::string& playlist_id = saved_items.front()->parents.front();
  const std::string& item_id = saved_items.front()->id;

  auto* side_panel_coordinator =
      PlaylistSidePanelCoordinator::FromBrowser(browser_);
  CHECK(side_panel_coordinator);
  side_panel_coordinator->ActivatePanel();

  // TODO(sko) Calling this will reload the web ui and we'll lose the video
  // being played if there is one. So if the panel has been already activated
  // and has something loaded, we should call web ui API and handle this from
  // the web ui side.
  side_panel_coordinator->LoadPlaylist(playlist_id, item_id);

  // Before closing widget, try resetting observer to avoid crash on Win11
  tab_helper_observation_.Reset();
  GetWidget()->Close();
}

void PlaylistEditBubbleView::ChangeFolder() {
  PlaylistActionDialog::Show<PlaylistMoveDialog>(
      static_cast<BrowserView*>(browser_->window()), tab_helper_.get());
}

void PlaylistEditBubbleView::RemoveFromPlaylist() {
  CHECK(tab_helper_);
  const auto& saved_items = tab_helper_->saved_items();
  CHECK(saved_items.size());

  std::vector<mojom::PlaylistItemPtr> items;
  base::ranges::transform(saved_items, std::back_inserter(items),
                          [](const auto& item) { return item->Clone(); });

  // Before closing widget, try resetting observer to avoid crash on Win11
  tab_helper_observation_.Reset();

  tab_helper_->RemoveItems(std::move(items));
  GetWidget()->Close();
}

void PlaylistEditBubbleView::MoreMediaInContents() {
  if (tab_helper_ && !tab_helper_->GetUnsavedItems().empty()) {
    next_bubble_ = PlaylistBubblesController::BubbleType::kAdd;
  }

  GetWidget()->Close();
}

BEGIN_METADATA(PlaylistEditBubbleView)
END_METADATA
}  // namespace playlist
