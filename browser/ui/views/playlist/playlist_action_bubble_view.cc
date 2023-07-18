/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_action_bubble_view.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/playlist/playlist_tab_helper.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
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
// DefaultThumbnailBackground
//
// The default thumbnail could be used in various ratio. So we use static image
// for foreground and draw background programmatically.
class DefaultThumbnailBackground : public views::Background {
 public:
  using Background::Background;
  ~DefaultThumbnailBackground() override = default;

  // views::Background:
  void Paint(gfx::Canvas* canvas, views::View* view) const override {
    const auto& bounds = view->GetContentsBounds();
    cc::PaintFlags flags;
    flags.setBlendMode(SkBlendMode::kSrcOver);
    flags.setShader(
        gfx::CreateGradientShader(bounds.bottom_right(), bounds.origin(),
                                  /*start=*/SkColorSetRGB(0x32, 0x2F, 0xB4),
                                  /*end*/ SkColorSetRGB(0x38, 0x35, 0xCA)));

    canvas->DrawRect(bounds, flags);
  }
};

////////////////////////////////////////////////////////////////////////////////
// ConfirmBubble
//  * Shows when items were added to the current page.
//  * Contains actions to manipulate items.
class ConfirmBubble : public PlaylistActionBubbleView {
 public:
  METADATA_HEADER(ConfirmBubble);

  ConfirmBubble(Browser* browser,
                PlaylistActionIconView* anchor,
                playlist::PlaylistTabHelper* playlist_tab_helper);
  ~ConfirmBubble() override = default;

 private:
  class Row : public views::LabelButton {
   public:
    Row(const std::u16string& text,
        const ui::ImageModel& icon,
        views::Button::PressedCallback callback = {});
    ~Row() override = default;

    // views::LabelButton:
    void Layout() override;
  };

  void OpenInPlaylist();
  void ChangeFolder();
  void RemoveFromPlaylist();
  void MoreMediaInContents();
};

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
  class ItemRow : public views::Button {
   public:
    static constexpr int kWidth = 288;

    ItemRow(playlist::mojom::PlaylistItemPtr item,
            base::RepeatingCallback<void(const ItemRow&)> callback);

    bool selected() const { return selected_; }
    const playlist::mojom::PlaylistItemPtr& item() const { return item_; }

    // views::Button:
    int GetHeightForWidth(int width) const override;
    void OnThemeChanged() override;

   private:
    ItemRow& OnPressed(const ui::Event& event);

    void SetSelected(bool selected);
    void UpdateBackground();

    playlist::mojom::PlaylistItemPtr item_;
    bool selected_ = true;

    raw_ptr<views::ImageView> selected_icon_ = nullptr;
  };

  void AddSelected();
  void OnItemPressed(const ItemRow& row);

  base::flat_set<const ItemRow*> selected_views_;
};

////////////////////////////////////////////////////////////////////////////////
// ConfirmBubble Impl

ConfirmBubble::Row::Row(const std::u16string& text,
                        const ui::ImageModel& icon,
                        views::Button::PressedCallback callback)
    : LabelButton(callback, text) {
  SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_RIGHT);
  SetImageModel(views::Button::STATE_NORMAL, icon);
  label()->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);
}

void ConfirmBubble::Row::Layout() {
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

  constexpr int kIconSize = 16;
  AddChildView(std::make_unique<Row>(
      l10n_util::GetStringUTF16(IDS_PLAYLIST_ADDED_TO_PLAY_LATER),
      ui::ImageModel::FromVectorIcon(kLeoCheckCircleFilledIcon,
                                     kColorBravePlaylistAddedIcon, kIconSize)));
  AddChildView(std::make_unique<views::Separator>());
  AddChildView(std::make_unique<Row>(
      l10n_util::GetStringUTF16(IDS_PLAYLIST_OPEN_IN_PLAYLIST),
      ui::ImageModel::FromVectorIcon(kLeoProductPlaylistIcon,
                                     ui::kColorMenuIcon, kIconSize),
      base::BindRepeating(&ConfirmBubble::OpenInPlaylist,
                          base::Unretained(this))));
  AddChildView(std::make_unique<Row>(
      l10n_util::GetStringUTF16(IDS_PLAYLIST_CHANGE_FOLDER),
      ui::ImageModel::FromVectorIcon(kLeoFolderExchangeIcon, ui::kColorMenuIcon,
                                     kIconSize),
      base::BindRepeating(&ConfirmBubble::ChangeFolder,
                          base::Unretained(this))));
  AddChildView(std::make_unique<Row>(
      l10n_util::GetStringUTF16(IDS_PLAYLIST_REMOVE_FROM_PLAYLIST),
      ui::ImageModel::FromVectorIcon(kLeoTrashIcon, ui::kColorMenuIcon,
                                     kIconSize),
      base::BindRepeating(&ConfirmBubble::RemoveFromPlaylist,
                          base::Unretained(this))));

  if (playlist_tab_helper->GetUnsavedItems().size()) {
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
  side_panel_coordinator->LoadPlaylist(playlist_id, item_id);
}

void ConfirmBubble::ChangeFolder() {
  NOTIMPLEMENTED();
}

void ConfirmBubble::RemoveFromPlaylist() {
  CHECK(playlist_tab_helper_);
  const auto& saved_items = playlist_tab_helper_->saved_items();
  CHECK(saved_items.size());

  std::vector<playlist::mojom::PlaylistItemPtr> items;
  base::ranges::transform(saved_items, std::back_inserter(items),
                          [](const auto& item) { return item->Clone(); });

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
AddBubble::ItemRow::ItemRow(
    playlist::mojom::PlaylistItemPtr item,
    base::RepeatingCallback<void(const ItemRow&)> callback)
    : Button(base::BindRepeating(&ItemRow::OnPressed, base::Unretained(this))
                 .Then(callback)),
      item_(std::move(item)) {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(8),
      /*between_child_spacing=*/16));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  SetPreferredSize(gfx::Size(288, 64));

  auto* thumbnail = AddChildView(std::make_unique<views::ImageView>());
  constexpr gfx::Size kThumbnailSize(64, 48);
  // TODO(sko) We can't set the item's thumbnail for now. We need some
  // prerequisite.
  //  * Download the thumbnail from network
  //  * Sanitize the image
  thumbnail->SetImage(
      ui::ImageModel::FromResourceId(IDR_PLAYLIST_DEFAULT_THUMBNAIL));
  thumbnail->SetBackground(std::make_unique<DefaultThumbnailBackground>());
  thumbnail->SetHorizontalAlignment(views::ImageViewBase::Alignment::kCenter);
  thumbnail->SetVerticalAlignment(views::ImageViewBase::Alignment::kCenter);
  thumbnail->SetPreferredSize(kThumbnailSize);
  thumbnail->SetImageSize({kThumbnailSize.height(), kThumbnailSize.height()});

  auto* title = AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(item_->name)));
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  layout->SetFlexForView(title, 1);

  selected_icon_ = AddChildView(std::make_unique<views::ImageView>());
  selected_icon_->SetImage(ui::ImageModel::FromVectorIcon(
      kLeoCheckCircleOutlineIcon, kColorBravePlaylistCheckedIcon, 20));
}

int AddBubble::ItemRow::GetHeightForWidth(int width) const {
  return GetPreferredSize().height();
}

void AddBubble::ItemRow::OnThemeChanged() {
  views::Button::OnThemeChanged();
  UpdateBackground();
}

AddBubble::ItemRow& AddBubble::ItemRow::OnPressed(const ui::Event& event) {
  SetSelected(!selected_);
  return *this;
}

void AddBubble::ItemRow::SetSelected(bool selected) {
  if (selected_ == selected) {
    return;
  }

  selected_ = selected;
  selected_icon_->SetVisible(selected_);

  UpdateBackground();
}

void AddBubble::ItemRow::UpdateBackground() {
  auto* cp = GetColorProvider();
  DCHECK(cp);

  if (selected_) {
    SetBackground(views::CreateSolidBackground(
        cp->GetColor(kColorBravePlaylistSelectedBackground)));
  } else {
    SetBackground(nullptr);
  }
}

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
    : PlaylistActionBubbleView(browser, anchor, playlist_tab_helper) {
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

  auto* contents =
      scroll_view->SetContents(std::make_unique<views::BoxLayoutView>());
  contents->SetOrientation(views::BoxLayout::Orientation::kVertical);

  for (const auto& item : items) {
    selected_views_.insert(contents->AddChildView(std::make_unique<ItemRow>(
        item.Clone(), base::BindRepeating(&AddBubble::OnItemPressed,
                                          base::Unretained(this)))));
  }

  // Fix preferred width. This is for ignoring insets that could be added by
  // border.
  scroll_view->SetPreferredSize(
      gfx::Size(ItemRow::kWidth, scroll_view->GetPreferredSize().height()));

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
  CHECK(selected_views_.size())
      << "The button should be disabled when nothing is selected.";

  if (playlist_tab_helper_->is_adding_items()) {
    // Don't do anything when already adding
    return;
  }

  std::vector<playlist::mojom::PlaylistItemPtr> items;
  base::ranges::transform(
      selected_views_, std::back_inserter(items),
      [](const auto* view) { return view->item().Clone(); });

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&playlist::PlaylistTabHelper::AddItems,
                     playlist_tab_helper_->GetWeakPtr(), std::move(items)));
}

void AddBubble::OnItemPressed(const ItemRow& row) {
  if (row.selected()) {
    selected_views_.insert(&row);
  } else {
    selected_views_.erase(&row);
  }

  if (bool has_selected = selected_views_.size();
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
