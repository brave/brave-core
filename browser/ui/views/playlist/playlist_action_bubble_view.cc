/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_action_bubble_view.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/browser/playlist/playlist_tab_helper.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/playlist/playlist_action_dialogs.h"
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/browser/ui/views/playlist/selectable_list_view.h"
#include "brave/browser/ui/views/playlist/thumbnail_provider.h"
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace {

PlaylistActionBubbleView* g_bubble = nullptr;

template <class ActionBubbleView,
          typename std::enable_if_t<
              std::is_base_of_v<PlaylistActionBubbleView, ActionBubbleView>>* =
              nullptr>
void ShowBubble(std::unique_ptr<ActionBubbleView> bubble) {
  DCHECK(!g_bubble);

  g_bubble = bubble.release();

  auto* widget = views::BubbleDialogDelegateView::CreateBubble(g_bubble);
  widget->Show();
}

////////////////////////////////////////////////////////////////////////////////
// ConfirmBubble
//  * Shows when items were added to the current page.
//  * Contains actions to manipulate items.
class ConfirmBubble : public PlaylistActionBubbleView,
                      public playlist::PlaylistTabHelperObserver {
 public:
  METADATA_HEADER(ConfirmBubble);

  ConfirmBubble(Browser* browser,
                PlaylistActionIconView* anchor,
                playlist::PlaylistTabHelper* playlist_tab_helper);
  ~ConfirmBubble() override = default;

  // PlaylistTabHelperObserver:
  void PlaylistTabHelperWillBeDestroyed() override;
  void OnSavedItemsChanged(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override;
  void OnFoundItemsChanged(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override {}
  void OnAddedItemFromTabHelper(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override {}

 private:
  void ResetChildViews();

  void OpenInPlaylist();
  void ChangeFolder();
  void RemoveFromPlaylist();
  void MoreMediaInContents();

  base::ScopedObservation<playlist::PlaylistTabHelper,
                          playlist::PlaylistTabHelperObserver>
      playlist_tab_helper_observation_{this};
};

class Row : public views::LabelButton {
  METADATA_HEADER(Row, views::LabelButton)
 public:
  Row(const std::u16string& text,
      const ui::ImageModel& icon,
      views::Button::PressedCallback callback = {});
  ~Row() override = default;

  // views::LabelButton:
  void Layout() override;
};

BEGIN_METADATA(Row)
END_METADATA

////////////////////////////////////////////////////////////////////////////////
// AddBubble
//  * Shows when users try adding items found from the current contents.
//  * Shows a list of found items and users can select which one to add.
class AddBubble : public PlaylistActionBubbleView {
 public:
  METADATA_HEADER(AddBubble);
  AddBubble(Browser* browser,
            PlaylistActionIconView* anchor,
            playlist::PlaylistTabHelper* playlist_tab_helper);
  AddBubble(Browser* browser,
            PlaylistActionIconView* anchor,
            playlist::PlaylistTabHelper* playlist_tab_helper,
            const std::vector<playlist::mojom::PlaylistItemPtr>& items);

 private:
  void AddSelected();
  void OnSelectionChanged();

  raw_ptr<SelectableItemsView> list_view_ = nullptr;

  std::unique_ptr<ThumbnailProvider> thumbnail_provider_;
};

////////////////////////////////////////////////////////////////////////////////
// ConfirmBubble Impl

Row::Row(const std::u16string& text,
         const ui::ImageModel& icon,
         views::Button::PressedCallback callback)
    : LabelButton(std::move(callback), text) {
  SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_RIGHT);
  SetImageModel(views::Button::STATE_NORMAL, icon);
  label()->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);
}

void Row::Layout() {
  LabelButton::Layout();
  // Extend |label|'s width so the this button's sub controls are justified.
  const auto contents_x = GetContentsBounds().x();
  label()->SetX(contents_x);
  label()->SetSize(
      {label()->width() + label()->x() - contents_x, label()->height()});
}

ConfirmBubble::ConfirmBubble(Browser* browser,
                             PlaylistActionIconView* anchor,
                             playlist::PlaylistTabHelper* playlist_tab_helper)
    : PlaylistActionBubbleView(browser, anchor, playlist_tab_helper) {
  // What this look like
  // https://user-images.githubusercontent.com/5474642/243532057-4bbbe779-47a1-4c3a-bd34-ce1334cf1d1d.png
  set_margins({});
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetLayoutManager(std::make_unique<views::BoxLayout>(
                       views::BoxLayout::Orientation::kVertical,
                       gfx::Insets::VH(4, 16),
                       /*between_child_spacing =*/4))
      ->set_cross_axis_alignment(
          views::BoxLayout::CrossAxisAlignment::kStretch);
  ResetChildViews();

  playlist_tab_helper_observation_.Observe(playlist_tab_helper_);
}

void ConfirmBubble::PlaylistTabHelperWillBeDestroyed() {
  playlist_tab_helper_observation_.Reset();
}

void ConfirmBubble::OnSavedItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>& items) {
  if (auto* widget = GetWidget(); !widget || widget->IsClosed()) {
    return;
  }

  ResetChildViews();
  SizeToContents();
}

void ConfirmBubble::ResetChildViews() {
  RemoveAllChildViews();

  constexpr int kIconSize = 16;
  // TODO(sko) There was feedback that "Added to Play Later" is pretty
  // confusing. For now we show "Added to Playlist" for clarity. When we
  // come to the conclusion, revert this to use
  // PlaylistTabHelper::GetSavedFolderName() if it's needed
  AddChildView(std::make_unique<Row>(
      l10n_util::GetStringFUTF16(IDS_PLAYLIST_ADDED_TO_PLAYLIST_FOLDER,
                                 u"Playlist"),
      ui::ImageModel::FromVectorIcon(kLeoCheckCircleFilledIcon,
                                     kColorBravePlaylistAddedIcon, kIconSize)));
  bool added_separator = false;
  auto add_separator_if_needed = [&added_separator, this] {
    if (added_separator) {
      return;
    }

    added_separator = !!AddChildView(std::make_unique<views::Separator>());
  };

  const auto& saved_items = playlist_tab_helper_->saved_items();

  if (saved_items.front()->parents.size()) {
    add_separator_if_needed();
    AddChildView(std::make_unique<Row>(
        l10n_util::GetStringUTF16(IDS_PLAYLIST_OPEN_IN_PLAYLIST),
        ui::ImageModel::FromVectorIcon(kLeoProductPlaylistIcon,
                                       ui::kColorMenuIcon, kIconSize),
        base::BindRepeating(&ConfirmBubble::OpenInPlaylist,
                            base::Unretained(this))));
  }

  if (PlaylistMoveDialog::CanMoveItems(saved_items)) {
    add_separator_if_needed();
    AddChildView(std::make_unique<Row>(
        l10n_util::GetStringUTF16(IDS_PLAYLIST_CHANGE_FOLDER),
        ui::ImageModel::FromVectorIcon(kLeoFolderExchangeIcon,
                                       ui::kColorMenuIcon, kIconSize),
        base::BindRepeating(&ConfirmBubble::ChangeFolder,
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
        base::BindRepeating(&ConfirmBubble::RemoveFromPlaylist,
                            base::Unretained(this))));
  }

  if (playlist_tab_helper_->GetUnsavedItems().size()) {
    AddChildView(std::make_unique<views::Separator>());
    AddChildView(std::make_unique<Row>(
        l10n_util::GetStringUTF16(IDS_PLAYLIST_MORE_MEDIA_IN_THIS_PAGE),
        ui::ImageModel::FromVectorIcon(kLeoProductPlaylistIcon,
                                       ui::kColorMenuIcon, kIconSize),
        base::BindRepeating(&ConfirmBubble::MoreMediaInContents,
                            base::Unretained(this))));
  }
}

void ConfirmBubble::OpenInPlaylist() {
  // Technically, the saved items could belong to multiple playlists
  // at the same time and their parent playlists could be different from each
  // other's. But for simplicity, we just open the first one assuming that most
  // users keep items from a site in a same playlist.
  const auto& saved_items = playlist_tab_helper_->saved_items();
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
  playlist_tab_helper_observation_.Reset();
  GetWidget()->Close();
}

void ConfirmBubble::ChangeFolder() {
  PlaylistActionDialog::Show<PlaylistMoveDialog>(
      static_cast<BrowserView*>(browser_->window()), playlist_tab_helper_);
}

void ConfirmBubble::RemoveFromPlaylist() {
  CHECK(playlist_tab_helper_);
  const auto& saved_items = playlist_tab_helper_->saved_items();
  CHECK(saved_items.size());

  std::vector<playlist::mojom::PlaylistItemPtr> items;
  base::ranges::transform(saved_items, std::back_inserter(items),
                          [](const auto& item) { return item->Clone(); });

  // Before closing widget, try resetting observer to avoid crash on Win11
  playlist_tab_helper_observation_.Reset();

  playlist_tab_helper_->RemoveItems(std::move(items));
  GetWidget()->Close();
}

void ConfirmBubble::MoreMediaInContents() {
  auto show_add_bubble = base::BindOnce(
      [](base::WeakPtr<playlist::PlaylistTabHelper> tab_helper,
         Browser* browser, base::WeakPtr<PlaylistActionIconView> anchor) {
        if (!tab_helper || !anchor) {
          return;
        }

        if (!tab_helper->found_items().size()) {
          return;
        }

        ::ShowBubble(
            std::make_unique<AddBubble>(browser, anchor.get(), tab_helper.get(),
                                        tab_helper->GetUnsavedItems()));
      },
      playlist_tab_helper_->GetWeakPtr(),
      // |Browser| outlives TabHelper so it's okay to bind raw ptr here
      browser_.get(), icon_view_->GetWeakPtr());

  SetCloseCallback(
      // WindowClosingImpl should be called first to clean up data before
      // showing up new bubble. This callback is called by itself, it's okay to
      // pass Unretained().
      base::BindOnce(&PlaylistActionBubbleView::WindowClosingImpl,
                     base::Unretained(this))
          .Then(std::move(show_add_bubble)));

  GetWidget()->Close();
}

BEGIN_METADATA(ConfirmBubble, PlaylistActionBubbleView)
END_METADATA

////////////////////////////////////////////////////////////////////////////////
// AddBubble Impl
AddBubble::AddBubble(Browser* browser,
                     PlaylistActionIconView* anchor,
                     playlist::PlaylistTabHelper* playlist_tab_helper)
    : AddBubble(browser,
                anchor,
                playlist_tab_helper,
                playlist_tab_helper->found_items()) {}

AddBubble::AddBubble(Browser* browser,
                     PlaylistActionIconView* anchor,
                     playlist::PlaylistTabHelper* playlist_tab_helper,
                     const std::vector<playlist::mojom::PlaylistItemPtr>& items)
    : PlaylistActionBubbleView(browser, anchor, playlist_tab_helper),
      thumbnail_provider_(
          std::make_unique<ThumbnailProvider>(playlist_tab_helper)) {
  // What this look like
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

  auto* scroll_view = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view->ClipHeightTo(/*min_height=*/0, /*max_height=*/230);
  scroll_view->SetDrawOverflowIndicator(false);
  scroll_view->SetBorder(views::CreateThemedRoundedRectBorder(
      /*thickness=*/1,
      /*corner_radius=*/4.f, kColorBravePlaylistListBorder));

  list_view_ = scroll_view->SetContents(std::make_unique<SelectableItemsView>(
      thumbnail_provider_.get(), items,
      base::BindRepeating(&AddBubble::OnSelectionChanged,
                          base::Unretained(this))));
  list_view_->SetSelected(items);

  // Fix preferred width. This is for ignoring insets that could be added by
  // border.
  static constexpr int kWidth = 288;
  scroll_view->SetPreferredSize(
      gfx::Size(kWidth, scroll_view->GetPreferredSize().height()));

  SetButtonLabel(ui::DialogButton::DIALOG_BUTTON_OK,
                 l10n_util::GetStringUTF16(IDS_PLAYLIST_ADD_SELECTED));

  // This callback is called by itself, it's okay to pass Unretained(this).
  SetAcceptCallback(base::BindOnce(&PlaylistActionBubbleView::WindowClosingImpl,
                                   base::Unretained(this))
                        .Then(base::BindOnce(&AddBubble::AddSelected,
                                             base::Unretained(this))));
}

void AddBubble::AddSelected() {
  CHECK(playlist_tab_helper_);

  if (playlist_tab_helper_->is_adding_items()) {
    // Don't do anything when already adding
    return;
  }

  std::vector<playlist::mojom::PlaylistItemPtr> items =
      list_view_->GetSelected();
  CHECK(items.size())
      << "The button should be disabled when nothing is selected.";

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&playlist::PlaylistTabHelper::AddItems,
                     playlist_tab_helper_->GetWeakPtr(), std::move(items)));
}

void AddBubble::OnSelectionChanged() {
  if (bool has_selected = list_view_->HasSelected();
      has_selected != IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK)) {
    SetButtonEnabled(ui::DIALOG_BUTTON_OK, has_selected);
  }
}

BEGIN_METADATA(AddBubble, PlaylistActionBubbleView)
END_METADATA

}  // namespace

// static
void PlaylistActionBubbleView::ShowBubble(
    Browser* browser,
    PlaylistActionIconView* anchor,
    playlist::PlaylistTabHelper* playlist_tab_helper) {
  if (playlist_tab_helper->saved_items().size()) {
    ::ShowBubble(
        std::make_unique<ConfirmBubble>(browser, anchor, playlist_tab_helper));
  } else if (playlist_tab_helper->found_items().size()) {
    ::ShowBubble(
        std::make_unique<AddBubble>(browser, anchor, playlist_tab_helper));
  } else {
    NOTREACHED() << "Caller should filter this case";
  }
}

// static
bool PlaylistActionBubbleView::IsShowingBubble() {
  return g_bubble && g_bubble->GetWidget() &&
         !g_bubble->GetWidget()->IsClosed();
}

// static
void PlaylistActionBubbleView::CloseBubble() {
  g_bubble->GetWidget()->Close();
}

// static
PlaylistActionBubbleView* PlaylistActionBubbleView::GetBubble() {
  return g_bubble;
}

PlaylistActionBubbleView::PlaylistActionBubbleView(
    Browser* browser,
    PlaylistActionIconView* anchor,
    playlist::PlaylistTabHelper* playlist_tab_helper)
    : BubbleDialogDelegateView(anchor, views::BubbleBorder::Arrow::TOP_RIGHT),
      browser_(browser),
      playlist_tab_helper_(playlist_tab_helper),
      icon_view_(anchor) {}

PlaylistActionBubbleView::~PlaylistActionBubbleView() = default;

void PlaylistActionBubbleView::WindowClosing() {
  BubbleDialogDelegateView::WindowClosing();

  WindowClosingImpl();
}

void PlaylistActionBubbleView::WindowClosingImpl() {
  // This method could be called multiple times during the closing process in
  // order to show up a subsequent action bubble. So we should check if
  // |g_bubble| is already filled up with a new bubble.
  if (g_bubble == this) {
    g_bubble = nullptr;
  }
}

BEGIN_METADATA(PlaylistActionBubbleView, views::BubbleDialogDelegateView)
END_METADATA
