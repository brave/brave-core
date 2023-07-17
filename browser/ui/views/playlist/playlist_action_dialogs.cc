/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_action_dialogs.h"

#include <string>
#include <utility>

#include "brave/browser/playlist/playlist_tab_helper.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/playlist/thumbnail_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/fill_layout.h"

namespace {

// This class takes PlaylistItems and show tiled thumbnail and the count of
// them. When only one item is passed, shows the title of it instead.
class TiledItemsView : public views::BoxLayoutView {
 public:
  METADATA_HEADER(TiledItemsView);

  static constexpr gfx::Size kThumbnailSize = gfx::Size(64, 48);
  static constexpr int kCornerRadius = 4;

  explicit TiledItemsView(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) {
    DCHECK_GE(items.size(), 1u);

    SetPreferredSize(gfx::Size(464, 72));
    SetBorder(views::CreateThemedRoundedRectBorder(
        /*thickness=*/1, kCornerRadius, kColorBravePlaylistListBorder));
    SetInsideBorderInsets(gfx::Insets(8));
    SetBetweenChildSpacing(16);

    AddChildView(CreateThumbnailTiles(items))->SetPreferredSize(kThumbnailSize);
    AddChildView(std::make_unique<views::Label>(
        items.size() == 1 ? base::UTF8ToUTF16(items.front()->name)
                          : l10n_util::GetStringFUTF16Int(
                                IDS_PLAYLIST_MOVE_MEDIA_DIALOG_SELECTED_ITEMS,
                                static_cast<int>(items.size()))));
  }

  std::unique_ptr<views::View> CreateThumbnailTiles(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) {
    constexpr size_t kMaxTileCount = 4;
    const bool is_single_row = items.size() < kMaxTileCount;

    auto container = std::make_unique<views::BoxLayoutView>();
    container->SetOrientation(views::BoxLayout::Orientation::kVertical);
    container->AddChildView(std::make_unique<views::BoxLayoutView>());
    SkPath clip;
    clip.addRoundRect(
        SkRect::MakeWH(kThumbnailSize.width(), kThumbnailSize.height()),
        kCornerRadius, kCornerRadius);
    container->SetClipPath(clip);
    if (!is_single_row) {
      container->AddChildView(std::make_unique<views::BoxLayoutView>());
    }

    const gfx::Size tile_size =
        is_single_row ? gfx::Size(kThumbnailSize.width() / items.size(),
                                  kThumbnailSize.height())
                      : gfx::Size(kThumbnailSize.width() / 2,
                                  kThumbnailSize.height() / 2);

    for (size_t i = 0; i < kMaxTileCount && i < items.size(); i++) {
      // TODO(sko) We can't set the item's thumbnail for now. We need some
      // prerequisite.
      //  * Download the thumbnail from network
      //  * Sanitize the image
      auto* row = i < kMaxTileCount / 2 ? container->children().front()
                                        : container->children().back();

      auto* thumbnail =
          row->AddChildView(std::make_unique<ThumbnailView>(gfx::Image()));
      thumbnail->SetPreferredSize(tile_size);
    }

    return container;
  }

  ~TiledItemsView() override = default;
};

BEGIN_METADATA(TiledItemsView, views::BoxLayoutView)
END_METADATA

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// PlaylistActionDialog
//
PlaylistActionDialog::PlaylistActionDialog() {
  SetShowTitle(true);
  SetShowCloseButton(false);
}

BEGIN_METADATA(PlaylistActionDialog, views::DialogDelegateView)
END_METADATA

////////////////////////////////////////////////////////////////////////////////
// PlaylistNewPlaylistDialog
//
PlaylistNewPlaylistDialog::PlaylistNewPlaylistDialog(PassKey) {
  NOTIMPLEMENTED();
}

BEGIN_METADATA(PlaylistNewPlaylistDialog, PlaylistActionDialog)
END_METADATA

////////////////////////////////////////////////////////////////////////////////
// PlaylistMoveDialog
//
// static
bool PlaylistMoveDialog::CanMoveItems(
    const std::vector<playlist::mojom::PlaylistItemPtr>& items) {
  // Technically, an item can have multiple parent Playlists, we only supports
  // this operation for items that have single parent Playlist.
  return base::ranges::all_of(
      items, [](const auto& item) { return item->parents.size() == 1u; });
}

void PlaylistMoveDialog::ContentsChanged(views::Textfield* sender,
                                         const std::u16string& new_contents) {
  if (new_contents.size() == IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK)) {
    return;
  }

  SetButtonEnabled(ui::DIALOG_BUTTON_OK, new_contents.size());
  DialogModelChanged();
}

void PlaylistMoveDialog::PlaylistTabHelperWillBeDestroyed() {
  if (auto* widget = GetWidget(); widget && !widget->IsClosed()) {
    GetWidget()->Close();
  }
}

void PlaylistMoveDialog::OnSavedItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>& items) {
  if (items.empty()) {
    if (auto* widget = GetWidget(); widget && !widget->IsClosed()) {
      GetWidget()->Close();
    }
    return;
  }

  // Rebuild views
  if (mode_ == Mode::kChoose) {
    EnterChoosePlaylistMode();
  } else if (mode_ == Mode::kCreate) {
    EnterCreatePlaylistMode();
  } else {
    NOTREACHED() << "If new mode was added, please revisit this.";
  }
}

PlaylistMoveDialog::PlaylistMoveDialog(PassKey,
                                       playlist::PlaylistTabHelper* tab_helper)
    : tab_helper_(tab_helper) {
  set_margins(gfx::Insets(24));

  const auto& items = tab_helper_->saved_items();

  DCHECK(items.size());
  DCHECK(CanMoveItems(items));

  SetTitle(l10n_util::GetStringUTF16(IDS_PLAYLIST_MOVE_MEDIA_DIALOG_TITLE));

  SetLayoutManager(std::make_unique<views::BoxLayout>(
                       views::BoxLayout::Orientation::kVertical))
      ->set_between_child_spacing(24);

  AddChildView(std::make_unique<TiledItemsView>(items));

  contents_container_ = AddChildView(std::make_unique<views::BoxLayoutView>());
  contents_container_->SetOrientation(views::BoxLayout::Orientation::kVertical);
  contents_container_->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  EnterChoosePlaylistMode();

  tab_helper_observation_.Observe(tab_helper_);
}

PlaylistMoveDialog::~PlaylistMoveDialog() = default;

void PlaylistMoveDialog::OnNewPlaylistPressed(const ui::Event& event) {
  EnterCreatePlaylistMode();
}

void PlaylistMoveDialog::OnBackPressed(const ui::Event& event) {
  EnterChoosePlaylistMode();
}

void PlaylistMoveDialog::EnterChoosePlaylistMode() {
  mode_ = Mode::kChoose;

  contents_container_->RemoveAllChildViews();
  new_playlist_name_textfield_ = nullptr;

  auto* description = contents_container_->AddChildView(
      std::make_unique<views::Label>(l10n_util::GetStringUTF16(
          IDS_PLAYLIST_MOVE_MEDIA_DIALOG_DESCRIPTION)));
  description->SetEnabledColorId(kColorBravePlaylistMoveDialogDescription);
  description->SetPreferredSize(gfx::Size(kContentsWidth, 17));
  description->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);

  auto* scroll_view =
      contents_container_->AddChildView(std::make_unique<views::ScrollView>());
  scroll_view->ClipHeightTo(/*min_height=*/0, /*max_height=*/224);
  scroll_view->SetDrawOverflowIndicator(false);
  scroll_view->SetBorder(views::CreateThemedRoundedRectBorder(
      /*thickness=*/1,
      /*corner_radius=*/4.f, kColorBravePlaylistListBorder));

  list_view_ =
      scroll_view->SetContents(std::make_unique<SelectablePlaylistsView>(
          tab_helper_->GetAllPlaylists(), base::DoNothing()));
  const auto& items = tab_helper_->saved_items();
  DCHECK_GE(items.size(), 1u);
  DCHECK_EQ(items.front()->parents.size(), 1u);

  std::string candidate_playlist_id = items.front()->parents.front();
  if (items.size() > 1u &&
      !std::all_of(items.begin() + 1, items.end(),
                   [&candidate_playlist_id](const auto& item) {
                     return item->parents.front() == candidate_playlist_id;
                   })) {
    // When items belong to different playlists, set the default playlist as
    // candidate.
    candidate_playlist_id = playlist::kDefaultPlaylistID;
  }
  list_view_->SetSelected({candidate_playlist_id});

  SetButtonLabel(ui::DIALOG_BUTTON_OK,
                 l10n_util::GetStringUTF16(IDS_PLAYLIST_MOVE_MEDIA_DIALOG_OK));

  // AcceptCallback is invoked by the base class so it's okay to bind
  // Unretained(this).
  SetAcceptCallback(base::BindOnce(&PlaylistMoveDialog::OnMoveToPlaylist,
                                   base::Unretained(this)));

  // This view owns the button so it's okay to bind Unretained(this).
  SetExtraView(std::make_unique<views::LabelButton>(
      base::BindRepeating(&PlaylistMoveDialog::OnNewPlaylistPressed,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(IDS_PLAYLIST_MOVE_MEDIA_DIALOG_NEW_PLAYLIST)));
  DialogModelChanged();
  SizeToPreferredSize();
}

void PlaylistMoveDialog::EnterCreatePlaylistMode() {
  mode_ = Mode::kCreate;

  contents_container_->RemoveAllChildViews();
  list_view_ = nullptr;

  auto* title = contents_container_->AddChildView(
      std::make_unique<views::Label>(l10n_util::GetStringUTF16(
          IDS_PLAYLIST_MOVE_MEDIA_DIALOG_PLAYLIST_NAME)));
  title->SetEnabledColorId(
      kColorBravePlaylistMoveDialogCreatePlaylistAndMoveTitle);
  title->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);

  new_playlist_name_textfield_ =
      contents_container_->AddChildView(std::make_unique<views::Textfield>());
  new_playlist_name_textfield_->SetPreferredSize(gfx::Size(kContentsWidth, 40));
  new_playlist_name_textfield_->set_controller(this);
  new_playlist_name_textfield_->RequestFocus();

  SetButtonEnabled(ui::DIALOG_BUTTON_OK, false);
  SetButtonLabel(ui::DIALOG_BUTTON_OK,
                 l10n_util::GetStringUTF16(
                     IDS_PLAYLIST_MOVE_MEDIA_DIALOG_CREATE_AND_MOVE));

  // AcceptCallback is invoked by the base class so it's okay to bind
  // Unretained(this).
  SetAcceptCallback(base::BindOnce(&PlaylistMoveDialog::OnCreatePlaylistAndMove,
                                   base::Unretained(this)));

  // This view owns the button so it's okay to bind Unretained(this).
  SetExtraView(std::make_unique<views::LabelButton>(
      base::BindRepeating(&PlaylistMoveDialog::OnBackPressed,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(IDS_PLAYLIST_MOVE_MEDIA_DIALOG_BACK)));

  DialogModelChanged();
  SizeToPreferredSize();
}

void PlaylistMoveDialog::SizeToPreferredSize() {
  if (auto* widget = GetWidget()) {
    widget->CenterWindow(widget->non_client_view()
                             ->GetWindowBoundsForClientBounds(gfx::Rect(
                                 widget->client_view()->GetPreferredSize()))
                             .size());
  }
}

void PlaylistMoveDialog::OnMoveToPlaylist() {
  DCHECK(list_view_);
  auto selected = list_view_->GetSelected();
  DCHECK_EQ(selected.size(), 1u);

  // Before doing operation, reset observation so that we don't try to rebuild
  // views. It's okay because this view is about to be closed.
  tab_helper_observation_.Reset();

  std::vector<playlist::mojom::PlaylistItemPtr> items;
  base::ranges::transform(tab_helper_->saved_items(), std::back_inserter(items),
                          [](const auto& item) { return item->Clone(); });
  tab_helper_->MoveItems(std::move(items), selected.front()->Clone());
}

void PlaylistMoveDialog::OnCreatePlaylistAndMove() {
  DCHECK(new_playlist_name_textfield_ &&
         new_playlist_name_textfield_->GetText().size());

  // Before doing operation, reset observation so that we don't try to rebuild
  // views. It's okay because this view is about to be closed.
  tab_helper_observation_.Reset();

  std::vector<playlist::mojom::PlaylistItemPtr> items;
  base::ranges::transform(tab_helper_->saved_items(), std::back_inserter(items),
                          [](const auto& item) { return item->Clone(); });
  tab_helper_->MoveItemsToNewPlaylist(
      std::move(items),
      base::UTF16ToUTF8(new_playlist_name_textfield_->GetText()));
}

BEGIN_METADATA(PlaylistMoveDialog, PlaylistActionDialog)
END_METADATA

////////////////////////////////////////////////////////////////////////////////
// PlaylistRemovePlaylistConfirmDialog
//
PlaylistRemovePlaylistConfirmDialog::PlaylistRemovePlaylistConfirmDialog(
    PassKey) {
  NOTIMPLEMENTED();
}

BEGIN_METADATA(PlaylistRemovePlaylistConfirmDialog, PlaylistActionDialog)
END_METADATA
