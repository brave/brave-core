/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_action_dialogs.h"

#include <string>
#include <utility>

#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/playlist/playlist_browser_finder.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/browser/ui/views/playlist/playlist_bubbles_controller.h"
#include "brave/browser/ui/views/playlist/thumbnail_view.h"
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/fill_layout.h"

namespace {

BrowserView* FindBrowserViewFromWebContents(content::WebContents* contents) {
  auto* browser = playlist::FindBrowserForPlaylistWebUI(contents);
  if (!browser) {
    return nullptr;
  }

  return BrowserView::GetBrowserViewForBrowser(browser);
}

bool CanMoveItem(const playlist::mojom::PlaylistItemPtr& item) {
  CHECK(item);

  // Technically, an item can have multiple parent Playlists, we only supports
  // this operation for items that have single parent Playlist.
  return item->parents.size() == 1u;
}

// This class takes PlaylistItems and show tiled thumbnail and the count of
// them. When only one item is passed, shows the title of it instead.
class TiledItemsView : public views::BoxLayoutView {
  METADATA_HEADER(TiledItemsView, views::BoxLayoutView)
 public:
  static constexpr gfx::Size kThumbnailSize = gfx::Size(64, 48);
  static constexpr int kCornerRadius = 4;

  TiledItemsView(const std::vector<playlist::mojom::PlaylistItemPtr>& items,
                 ThumbnailProvider* thumbnail_provider)
      : thumbnail_provider_(thumbnail_provider) {
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
      views::View* row = i < kMaxTileCount / 2 ? container->children().front()
                                               : container->children().back();

      auto* thumbnail =
          row->AddChildView(std::make_unique<ThumbnailView>(gfx::Image()));
      thumbnail_provider_->GetThumbnail(items[i],
                                        thumbnail->GetThumbnailSetter());
      thumbnail->SetPreferredSize(tile_size);
    }

    return container;
  }

  ~TiledItemsView() override = default;

  raw_ptr<ThumbnailProvider> thumbnail_provider_;
};

BEGIN_METADATA(TiledItemsView)
END_METADATA

// A textfield that limits the maximum length of the input text.
class BoundedTextfield : public views::Textfield {
  METADATA_HEADER(BoundedTextfield, views::Textfield)

 public:
  explicit BoundedTextfield(size_t max_length) : max_length_(max_length) {
    length_label_ = AddChildView(std::make_unique<views::Label>());
    length_label_->SetHorizontalAlignment(
        gfx::HorizontalAlignment::ALIGN_RIGHT);
    UpdateLengthLabel();
  }
  ~BoundedTextfield() override = default;

  // views::Textfield:
  void OnTextChanged() override {
    Textfield::OnTextChanged();
    UpdateLengthLabel();

    // Double check the result as users can change contents via paste or
    // composition.
    // Note that this will be done in the next tick so that composition can
    // finish its job.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&BoundedTextfield::TruncateText,
                                  weak_ptr_factory_.GetWeakPtr()));
  }

  void InsertChar(const ui::KeyEvent& event) override {
    if (GetText().size() >= max_length_) {
      return;
    }

    Textfield::InsertChar(event);
  }

  void Layout(PassKey) override {
    LayoutSuperclass<views::Textfield>(this);

    length_label_->SetBoundsRect(GetContentsBounds());
  }

 private:
  void TruncateText() {
    if (auto text = GetText(); text.length() > max_length_) {
      SetText({text.begin(), text.begin() + max_length_});
    }
  }

  void UpdateLengthLabel() {
    length_label_->SetText(base::UTF8ToUTF16(
        base::StringPrintf("%zu/%zu", GetText().length(), max_length_)));
  }

  const size_t max_length_;

  raw_ptr<views::Label> length_label_;

  base::WeakPtrFactory<BoundedTextfield> weak_ptr_factory_{this};
};

BEGIN_METADATA(BoundedTextfield)
END_METADATA

}  // namespace

namespace playlist {
void ShowCreatePlaylistDialog(content::WebContents* contents) {
  DVLOG(2) << __FUNCTION__;
  PlaylistActionDialog::Show<PlaylistNewPlaylistDialog>(
      FindBrowserViewFromWebContents(contents),
      playlist::PlaylistServiceFactory::GetForBrowserContext(
          contents->GetBrowserContext()));
}

void ShowRemovePlaylistDialog(content::WebContents* contents,
                              const std::string& playlist_id) {
  DVLOG(2) << __FUNCTION__;
  PlaylistActionDialog::Show<PlaylistRemovePlaylistConfirmDialog>(
      FindBrowserViewFromWebContents(contents),
      playlist::PlaylistServiceFactory::GetForBrowserContext(
          contents->GetBrowserContext()),
      playlist_id);
}

void ShowMoveItemsDialog(content::WebContents* contents,
                         const std::string& playlist_id,
                         const std::vector<std::string>& items) {
  DVLOG(2) << __FUNCTION__;
  PlaylistMoveDialog::MoveParam param;
  param.service = playlist::PlaylistServiceFactory::GetForBrowserContext(
      contents->GetBrowserContext());
  param.playlist_id = playlist_id;
  base::ranges::copy(items, std::back_inserter(param.items));

  PlaylistActionDialog::Show<PlaylistMoveDialog>(
      FindBrowserViewFromWebContents(contents), std::move(param));
}

void ShowPlaylistSettings(content::WebContents* contents) {
  auto* browser_view = FindBrowserViewFromWebContents(contents);
  CHECK(browser_view);
  ShowSingletonTab(browser_view->browser(),
                   GURL("brave://settings/braveContent#playlist-section"));
}

void ShowPlaylistAddBubble(content::WebContents* contents) {
  auto* browser_view = FindBrowserViewFromWebContents(contents);
  CHECK(browser_view);

  auto* tab_strip_model = browser_view->browser()->tab_strip_model();
  auto* playlist_tab_helper = PlaylistTabHelper::FromWebContents(
      tab_strip_model->GetActiveWebContents());
  if (playlist_tab_helper->found_items().empty()) {
    return;
  }

  static_cast<BraveLocationBarView*>(browser_view->GetLocationBarView())
      ->ShowPlaylistBubble(PlaylistBubblesController::BubbleType::kAdd);
}

void ClosePanel(content::WebContents* contents) {
  auto* browser_view = FindBrowserViewFromWebContents(contents);
  CHECK(browser_view);
  // TODO(): If this not opened in the Side Panel, we should consider
  // closing the tab.
  if (SidePanelUI* ui =
          browser_view->browser()->GetFeatures().side_panel_ui()) {
    ui->Close();
  }
}

}  // namespace playlist

////////////////////////////////////////////////////////////////////////////////
// PlaylistActionDialog
//
PlaylistActionDialog::PlaylistActionDialog() {
  SetShowTitle(true);
  SetShowCloseButton(false);
}

PlaylistActionDialog::~PlaylistActionDialog() = default;

BEGIN_METADATA(PlaylistActionDialog)
END_METADATA

////////////////////////////////////////////////////////////////////////////////
// PlaylistNewPlaylistDialog
//
PlaylistNewPlaylistDialog::PlaylistNewPlaylistDialog(
    PassKey,
    playlist::PlaylistService* service)
    : service_(service) {
  thumbnail_provider_ = std::make_unique<ThumbnailProvider>(*service_);
  const auto kSpacing = 24;
  SetBorder(views::CreateEmptyBorder(gfx::Insets(kSpacing)));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
                       views::BoxLayout::Orientation::kVertical, gfx::Insets(),
                       /* between_child_spacing=*/kSpacing))
      ->set_cross_axis_alignment(
          views::BoxLayout::CrossAxisAlignment::kStretch);
  SetTitle(l10n_util::GetStringUTF16(IDS_PLAYLIST_NEW_PLAYLIST_DIALOG_TITLE));
  SetButtonLabel(
      ui::DIALOG_BUTTON_OK,
      l10n_util::GetStringUTF16(IDS_PLAYLIST_NEW_PLAYLIST_DIALOG_OK));
  SetButtonEnabled(ui::DIALOG_BUTTON_OK, false);

  auto create_container = [](views::View* parent, int container_label_string_id,
                             int container_label_color_id,
                             int container_label_font_size) {
    auto* container =
        parent->AddChildView(std::make_unique<views::BoxLayoutView>());
    container->SetOrientation(views::BoxLayout::Orientation::kVertical);
    container->SetCrossAxisAlignment(
        views::BoxLayout::CrossAxisAlignment::kStretch);
    auto* container_label =
        container->AddChildView(std::make_unique<views::Label>(
            l10n_util::GetStringUTF16(container_label_string_id)));
    container_label->SetHorizontalAlignment(
        gfx::HorizontalAlignment::ALIGN_LEFT);
    container_label->SetFontList(
        container_label->font_list().DeriveWithSizeDelta(
            container_label->font_list().GetFontSize() -
            container_label_font_size));
    container_label->SetEnabledColorId(container_label_color_id);
    return container;
  };

  auto* name_field_container = AddChildView(
      create_container(this, IDS_PLAYLIST_NEW_PLAYLIST_DIALOG_NAME_TEXTFIELD,
                       kColorBravePlaylistNewPlaylistDialogNameLabel,
                       /* container_label_font_size=*/13));
  name_textfield_ = name_field_container->AddChildView(
      std::make_unique<BoundedTextfield>(/* max_length= */ 30u));
  name_textfield_->SetPreferredSize(gfx::Size(464, 39));
  name_textfield_->set_controller(this);

  auto default_playlist = service_->GetPlaylist(playlist::kDefaultPlaylistID);
  std::vector<playlist::mojom::PlaylistItemPtr> movable_items;
  for (const auto& item : default_playlist->items) {
    if (CanMoveItem(item)) {
      movable_items.push_back(item.Clone());
    }
  }

  if (movable_items.size()) {
    auto* items_list_view_container = AddChildView(create_container(
        this, IDS_PLAYLIST_NEW_PLAYLIST_DIALOG_SELECTABLE_ITEMS,
        kColorBravePlaylistNewPlaylistDialogItemsLabel,
        /* container_label_font_size=*/14));

    auto* scroll_view = items_list_view_container->AddChildView(
        std::make_unique<views::ScrollView>());
    scroll_view->ClipHeightTo(/*min_height=*/0, /*max_height=*/224);
    scroll_view->SetDrawOverflowIndicator(false);
    scroll_view->SetBorder(views::CreateThemedRoundedRectBorder(
        /*thickness=*/1,
        /*corner_radius=*/4.f, kColorBravePlaylistListBorder));

    items_list_view_ =
        scroll_view->SetContents(std::make_unique<SelectableItemsView>(
            thumbnail_provider_.get(), default_playlist->items,
            base::DoNothing()));
  }

  // It's okay to bind Unretained(this) as this callback is invoked by the base
  // class.
  SetAcceptCallback(base::BindOnce(&PlaylistNewPlaylistDialog::CreatePlaylist,
                                   base::Unretained(this)));
}

PlaylistNewPlaylistDialog::~PlaylistNewPlaylistDialog() = default;

views::View* PlaylistNewPlaylistDialog::GetInitiallyFocusedView() {
  return name_textfield_;
}

void PlaylistNewPlaylistDialog::ContentsChanged(
    views::Textfield* sender,
    const std::u16string& new_contents) {
  if (new_contents.size() == IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK)) {
    return;
  }

  SetButtonEnabled(ui::DIALOG_BUTTON_OK, new_contents.size());
  DialogModelChanged();
}

void PlaylistNewPlaylistDialog::CreatePlaylist() {
  DCHECK(!name_textfield_->GetText().empty());

  auto new_playlist = playlist::mojom::Playlist::New();
  new_playlist->name = base::UTF16ToUTF8(name_textfield_->GetText());
  if (items_list_view_ && items_list_view_->HasSelected()) {
    auto on_create_playlist = base::BindOnce(
        [](base::WeakPtr<playlist::PlaylistService> service,
           std::vector<playlist::mojom::PlaylistItemPtr> items_to_move,
           playlist::mojom::PlaylistPtr created_playlist) {
          CHECK(service);
          CHECK(created_playlist && created_playlist->id.has_value());
          for (const auto& item : items_to_move) {
            service->MoveItem(playlist::kDefaultPlaylistID,
                              created_playlist->id.value(), item->id);
          }
        },
        service_->GetWeakPtr(), items_list_view_->GetSelected());
    service_->CreatePlaylist(std::move(new_playlist),
                             std::move(on_create_playlist));
  } else {
    service_->CreatePlaylist(std::move(new_playlist), base::DoNothing());
  }
}

BEGIN_METADATA(PlaylistNewPlaylistDialog)
END_METADATA

////////////////////////////////////////////////////////////////////////////////
// PlaylistMoveDialog
//
// static
bool PlaylistMoveDialog::CanMoveItems(
    const std::vector<playlist::mojom::PlaylistItemPtr>& items) {
  return base::ranges::all_of(items, &CanMoveItem);
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
    NOTREACHED_IN_MIGRATION() << "If new mode was added, please revisit this.";
  }
}

PlaylistMoveDialog::MoveParam::MoveParam() = default;
PlaylistMoveDialog::MoveParam::MoveParam(MoveParam&&) = default;
PlaylistMoveDialog::MoveParam& PlaylistMoveDialog::MoveParam::operator=(
    MoveParam&&) = default;
PlaylistMoveDialog::MoveParam::~MoveParam() = default;

PlaylistMoveDialog::PlaylistMoveDialog(PassKey,
                                       playlist::PlaylistTabHelper* tab_helper)
    : PlaylistMoveDialog(raw_ptr(tab_helper)) {}

PlaylistMoveDialog::PlaylistMoveDialog(PassKey, MoveParam param)
    : PlaylistMoveDialog(std::move(param)) {}

PlaylistMoveDialog::PlaylistMoveDialog(
    absl::variant<raw_ptr<playlist::PlaylistTabHelper>, MoveParam> source)
    : source_(std::move(source)) {
  thumbnail_provider_ =
      is_from_tab_helper()
          ? std::make_unique<ThumbnailProvider>(get_tab_helper().get())
          : std::make_unique<ThumbnailProvider>(*get_move_param().service);

  set_margins(gfx::Insets(24));

  SetTitle(l10n_util::GetStringUTF16(IDS_PLAYLIST_MOVE_MEDIA_DIALOG_TITLE));

  SetLayoutManager(std::make_unique<views::BoxLayout>(
                       views::BoxLayout::Orientation::kVertical))
      ->set_between_child_spacing(24);

  if (is_from_tab_helper()) {
    const auto& items = get_tab_helper()->saved_items();
    DCHECK(items.size());
    AddChildView(
        std::make_unique<TiledItemsView>(items, thumbnail_provider_.get()));
  } else {
    auto service = get_move_param().service;
    std::vector<playlist::mojom::PlaylistItemPtr> items;
    for (const auto& item_id : get_move_param().items) {
      items.push_back(service->GetPlaylistItem(item_id));
    }
    AddChildView(
        std::make_unique<TiledItemsView>(items, thumbnail_provider_.get()));
  }

  contents_container_ = AddChildView(std::make_unique<views::BoxLayoutView>());
  contents_container_->SetOrientation(views::BoxLayout::Orientation::kVertical);
  contents_container_->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  EnterChoosePlaylistMode();

  if (is_from_tab_helper()) {
    tab_helper_observation_.Observe(get_tab_helper());
  }
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
          thumbnail_provider_.get(),
          is_from_tab_helper() ? get_tab_helper()->GetAllPlaylists()
                               : get_move_param().service->GetAllPlaylists(),
          base::DoNothing()));

  std::string candidate_playlist_id;
  if (is_from_tab_helper()) {
    const auto& items = get_tab_helper()->saved_items();
    DCHECK_GE(items.size(), 1u);
    DCHECK_EQ(items.front()->parents.size(), 1u);

    candidate_playlist_id = items.front()->parents.front();
    if (items.size() > 1u &&
        !std::all_of(items.begin() + 1, items.end(),
                     [&candidate_playlist_id](const auto& item) {
                       return item->parents.front() == candidate_playlist_id;
                     })) {
      // When items belong to different playlists, set the default playlist as
      // candidate.
      candidate_playlist_id = playlist::kDefaultPlaylistID;
    }
  } else {
    candidate_playlist_id = get_move_param().playlist_id;
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

  if (is_from_tab_helper()) {
    // Before doing operation, reset observation so that we don't try to
    // rebuild views. It's okay because this view is about to be closed.
    tab_helper_observation_.Reset();
    std::vector<playlist::mojom::PlaylistItemPtr> items;
    base::ranges::transform(get_tab_helper()->saved_items(),
                            std::back_inserter(items),
                            [](const auto& item) { return item->Clone(); });
    get_tab_helper()->MoveItems(std::move(items), selected.front()->Clone());
  } else {
    auto& move_param = get_move_param();
    for (const auto& item_id : move_param.items) {
      move_param.service->MoveItem(
          move_param.playlist_id,
          /* to_playlist_id= */ selected.front()->id.value(), item_id);
    }
  }
}

void PlaylistMoveDialog::OnCreatePlaylistAndMove() {
  DCHECK(new_playlist_name_textfield_ &&
         new_playlist_name_textfield_->GetText().size());

  if (is_from_tab_helper()) {
    // Before doing operation, reset observation so that we don't try to
    // rebuild views. It's okay because this view is about to be closed.
    tab_helper_observation_.Reset();

    std::vector<playlist::mojom::PlaylistItemPtr> items;
    base::ranges::transform(get_tab_helper()->saved_items(),
                            std::back_inserter(items),
                            [](const auto& item) { return item->Clone(); });
    get_tab_helper()->MoveItemsToNewPlaylist(
        std::move(items),
        base::UTF16ToUTF8(new_playlist_name_textfield_->GetText()));
  } else {
    auto service = get_move_param().service;
    auto on_create_playlist = base::BindOnce(
        [](MoveParam param, base::WeakPtr<playlist::PlaylistService> service,
           playlist::mojom::PlaylistPtr target_playlist) {
          if (!service) {
            return;
          }

          if (!target_playlist) {
            LOG(ERROR) << "Failed to create a new playlist before moving items "
                          "to it";
            return;
          }

          for (const auto& item_id : param.items) {
            service->MoveItem(param.playlist_id,
                              /* to_playlist_id= */ target_playlist->id.value(),
                              item_id);
          }
        },
        std::move(get_move_param()), service->GetWeakPtr());

    auto new_playlist = playlist::mojom::Playlist::New();
    new_playlist->name =
        base::UTF16ToUTF8(new_playlist_name_textfield_->GetText());
    service->CreatePlaylist(std::move(new_playlist),
                            std::move(on_create_playlist));
  }
}

BEGIN_METADATA(PlaylistMoveDialog)
END_METADATA

////////////////////////////////////////////////////////////////////////////////
// PlaylistRemovePlaylistConfirmDialog
//
PlaylistRemovePlaylistConfirmDialog::PlaylistRemovePlaylistConfirmDialog(
    PassKey,
    playlist::PlaylistService* service,
    const std::string& playlist_id)
    : service_(service), playlist_id_(playlist_id) {
  SetBorder(views::CreateEmptyBorder(gfx::Insets(24)));
  SetLayoutManager(std::make_unique<views::FillLayout>());
  SetTitle(
      l10n_util::GetStringUTF16(IDS_PLAYLIST_REMOVE_PLAYLIST_DIALOG_TITLE));
  SetButtonLabel(
      ui::DIALOG_BUTTON_OK,
      l10n_util::GetStringUTF16(IDS_PLAYLIST_REMOVE_PLAYLIST_DIALOG_OK));

  auto* description =
      AddChildView(std::make_unique<views::Label>(l10n_util::GetStringUTF16(
          IDS_PLAYLIST_REMOVE_PLAYLIST_DIALOG_DESCRIPTION)));
  constexpr auto kDescriptionMaxWidth = 312;
  description->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);

  if (description->GetPreferredSize().width() > kDescriptionMaxWidth) {
    description->SetMaximumWidthSingleLine(kDescriptionMaxWidth);
    description->SetMultiLine(true);
    // As views::Label::CalculatePreferred() size depends on the current
    // width of it, sets default size so that we can get proper size. If
    // line breaking makes the preferred size smaller than
    // kDescriptionMaxWidth, the description will be resized based on
    // that by the non client frame view.
    description->SetSize({kDescriptionMaxWidth, 0});
  }

  // It's okay to bind Unretained(this) as this callback is invoked by
  // the base class.
  SetAcceptCallback(
      base::BindOnce(&PlaylistRemovePlaylistConfirmDialog::RemovePlaylist,
                     base::Unretained(this)));
}

void PlaylistRemovePlaylistConfirmDialog::RemovePlaylist() {
  service_->RemovePlaylist(playlist_id_);
}

BEGIN_METADATA(PlaylistRemovePlaylistConfirmDialog)
END_METADATA
