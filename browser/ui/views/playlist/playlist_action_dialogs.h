/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_DIALOGS_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_DIALOGS_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/browser/ui/views/playlist/selectable_list_view.h"
#include "brave/components/playlist/browser/playlist_tab_helper_observer.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/window/dialog_delegate.h"

namespace views {
class BoxLayoutView;
}  // namespace views

namespace playlist {
class PlaylistTabHelper;
class PlaylistService;
}  // namespace playlist

// Base class for playlist action dialogs. Responsible for creating widget
// and anchoring.
class PlaylistActionDialog : public views::DialogDelegateView {
  METADATA_HEADER(PlaylistActionDialog, views::DialogDelegateView)
 public:

  template <class Dialog, typename... Args>
  static void Show(BrowserView* browser_view, Args&&... args) {
    DCHECK(browser_view);
    auto* browser_widget = browser_view->GetWidget();
    DCHECK(browser_widget);

    auto dialog = std::make_unique<Dialog>(typename Dialog::PassKey(),
                                           std::forward<Args>(args)...);
    dialog->SetModalType(ui::mojom::ModalType::kWindow);
    constrained_window::CreateBrowserModalDialogViews(
        std::move(dialog), browser_widget->GetNativeWindow())
        ->Show();
  }

  ~PlaylistActionDialog() override;

 protected:
  PlaylistActionDialog();

  std::unique_ptr<ThumbnailProvider> thumbnail_provider_;
};

class PlaylistNewPlaylistDialog : public PlaylistActionDialog,
                                  public views::TextfieldController {
  METADATA_HEADER(PlaylistNewPlaylistDialog, PlaylistActionDialog)
 public:

  using PassKey = base::PassKey<PlaylistActionDialog>;

  PlaylistNewPlaylistDialog(PassKey, playlist::PlaylistService* service);
  ~PlaylistNewPlaylistDialog() override;

  // PlaylistActionDialog:
  views::View* GetInitiallyFocusedView() override;

  // views::TextfieldController:
  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

 private:
  void CreatePlaylist();

  raw_ptr<playlist::PlaylistService> service_ = nullptr;

  raw_ptr<views::Textfield> name_textfield_ = nullptr;
  raw_ptr<SelectableItemsView> items_list_view_ = nullptr;
};

class PlaylistMoveDialog : public PlaylistActionDialog,
                           public views::TextfieldController,
                           public playlist::PlaylistTabHelperObserver {
  METADATA_HEADER(PlaylistMoveDialog, PlaylistActionDialog)
 public:

  using PassKey = base::PassKey<PlaylistActionDialog>;

  struct MoveParam {
    MoveParam();
    MoveParam(MoveParam&&);
    MoveParam& operator=(MoveParam&&);
    ~MoveParam();

    raw_ptr<playlist::PlaylistService> service;
    std::string playlist_id;
    std::vector<std::string> items;
  };

  PlaylistMoveDialog(PassKey, playlist::PlaylistTabHelper* tab_helper);
  PlaylistMoveDialog(PassKey, MoveParam param);
  ~PlaylistMoveDialog() override;

  static bool CanMoveItems(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items);

  // views::TextfieldController:
  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

  // playlist::PlaylistTabHelperObserver:
  void PlaylistTabHelperWillBeDestroyed() override;
  void OnSavedItemsChanged(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override;

 private:
  static constexpr int kContentsWidth = 464;

  enum class Mode {
    kChoose,
    kCreate,
  };

  explicit PlaylistMoveDialog(
      absl::variant<raw_ptr<playlist::PlaylistTabHelper>, MoveParam> source);

  void OnNewPlaylistPressed(const ui::Event& event);
  void OnBackPressed(const ui::Event& event);

  void EnterChoosePlaylistMode();
  void EnterCreatePlaylistMode();

  void SizeToPreferredSize();

  void OnMoveToPlaylist();
  void OnCreatePlaylistAndMove();

  bool is_from_tab_helper() const {
    return absl::holds_alternative<raw_ptr<playlist::PlaylistTabHelper>>(
        source_);
  }
  raw_ptr<playlist::PlaylistTabHelper> get_tab_helper() {
    return absl::get<raw_ptr<playlist::PlaylistTabHelper>>(source_);
  }
  MoveParam& get_move_param() { return absl::get<MoveParam>(source_); }

  absl::variant<raw_ptr<playlist::PlaylistTabHelper>, MoveParam> source_;

  Mode mode_ = Mode::kChoose;

  raw_ptr<views::BoxLayoutView> contents_container_ = nullptr;
  raw_ptr<SelectablePlaylistsView> list_view_ = nullptr;
  raw_ptr<views::Textfield> new_playlist_name_textfield_ = nullptr;

  base::ScopedObservation<playlist::PlaylistTabHelper,
                          playlist::PlaylistTabHelperObserver>
      tab_helper_observation_{this};
};

class PlaylistRemovePlaylistConfirmDialog : public PlaylistActionDialog {
  METADATA_HEADER(PlaylistRemovePlaylistConfirmDialog, PlaylistActionDialog)
 public:

  using PassKey = base::PassKey<PlaylistActionDialog>;

  PlaylistRemovePlaylistConfirmDialog(PassKey,
                                      playlist::PlaylistService* service,
                                      const std::string& playlist_id);
  ~PlaylistRemovePlaylistConfirmDialog() override = default;

 private:
  void RemovePlaylist();

  raw_ptr<playlist::PlaylistService> service_ = nullptr;
  const std::string playlist_id_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_DIALOGS_H_
