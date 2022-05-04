/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_items_contents_view.h"

#include <string>

#include "base/bind.h"
#include "base/i18n/case_conversion.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_added_feedback_bubble.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/sidebar/pref_names.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/event_utils.h"
#include "components/prefs/pref_service.h"
#include "ui/base/default_style.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/base/window_open_disposition.h"
#include "ui/compositor/compositor.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/image/image_skia_rep.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/background.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"

namespace {

constexpr gfx::Size kIconSize = {22, 22};

std::string GetFirstCharFromURL(const GURL& url) {
  DCHECK(url.is_valid());

  std::string target = url.host();
  size_t pos = target.find("www.");
  if (pos != std::string::npos && pos == 0) {
    target = target.substr(4, 1);
  } else {
    target = target.substr(0, 1);
  }
  return target;
}

sidebar::SidebarService* GetSidebarService(Browser* browser) {
  return sidebar::SidebarServiceFactory::GetForProfile(browser->profile());
}

}  // namespace

SidebarItemsContentsView::SidebarItemsContentsView(
    BraveBrowser* browser,
    views::DragController* drag_controller)
    : browser_(browser),
      drag_controller_(drag_controller),
      sidebar_model_(browser->sidebar_controller()->model()) {
  DCHECK(browser_);
  set_context_menu_controller(this);
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
}

SidebarItemsContentsView::~SidebarItemsContentsView() = default;

gfx::Size SidebarItemsContentsView::CalculatePreferredSize() const {
  if (children().empty())
    return {0, 0};
  const gfx::Size child_size = children()[0]->GetPreferredSize();
  return gfx::Size(
      child_size.width() + GetInsets().width(),
      children().size() * child_size.height() + GetInsets().height());
}

void SidebarItemsContentsView::OnThemeChanged() {
  View::OnThemeChanged();

  // BuiltIn items use different icon set based on theme.
  UpdateAllBuiltInItemsViewState();
}

void SidebarItemsContentsView::Update() {
  UpdateAllBuiltInItemsViewState();
}

void SidebarItemsContentsView::UpdateAllBuiltInItemsViewState() {
  const auto items = sidebar_model_->GetAllSidebarItems();
  // It's not initialized yet if child view count and items size are different.
  if (children().size() != items.size())
    return;

  const int active_index = sidebar_model_->active_index();
  const int items_num = items.size();
  for (int item_index = 0; item_index < items_num; ++item_index) {
    const auto item = items[item_index];
    if (!sidebar::IsBuiltInType(item))
      continue;

    // If browser window has tab that loads brave talk, brave talk panel icon
    // will use colored one for normal state also.
    if (item.built_in_item_type ==
        sidebar::SidebarItem::BuiltInItemType::kBraveTalk) {
      UpdateItemViewStateAt(
          item_index,
          browser_->sidebar_controller()->DoesBrowserHaveOpenedTabForItem(
              item));
      continue;
    }

    UpdateItemViewStateAt(item_index, item_index == active_index);
  }
}

std::u16string SidebarItemsContentsView::GetTooltipTextFor(
    const views::View* view) const {
  int index = GetIndexOf(view);
  DCHECK_GE(index, 0);

  if (index == -1)
    return std::u16string();

  auto item = sidebar_model_->GetAllSidebarItems()[index];
  if (!item.title.empty())
    return item.title;

  return base::UTF8ToUTF16(item.url.spec());
}

void SidebarItemsContentsView::ShowContextMenuForViewImpl(
    views::View* source,
    const gfx::Point& point,
    ui::MenuSourceType source_type) {
  if (context_menu_runner_ && context_menu_runner_->IsRunning())
    return;

  if (GetIndexOf(source) == -1)
    return;

  view_for_context_menu_ = source;
  context_menu_model_ = std::make_unique<ui::SimpleMenuModel>(this);
  SkColor icon_color = SK_ColorWHITE;
  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    icon_color = theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_BUTTON_BASE);
  }
  context_menu_model_->AddItemWithIcon(
      kItemRemove,
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_ITEM_CONTEXT_MENU_REMOVE),
      ui::ImageModel::FromVectorIcon(kSidebarTrashIcon, icon_color));
  context_menu_runner_ = std::make_unique<views::MenuRunner>(
      context_menu_model_.get(), views::MenuRunner::CONTEXT_MENU,
      base::BindRepeating(&SidebarItemsContentsView::OnContextMenuClosed,
                          base::Unretained(this)));
  context_menu_runner_->RunMenuAt(
      source->GetWidget(), nullptr, gfx::Rect(point, gfx::Size()),
      views::MenuAnchorPosition::kTopLeft, source_type);
}

void SidebarItemsContentsView::ExecuteCommand(int command_id, int event_flags) {
  int index = GetIndexOf(view_for_context_menu_);
  DCHECK_GE(index, 0);

  if (index == -1)
    return;

  if (command_id == kItemRemove) {
    GetSidebarService(browser_)->RemoveItemAt(index);
    return;
  }
}

void SidebarItemsContentsView::OnContextMenuClosed() {
  view_for_context_menu_ = nullptr;
}

void SidebarItemsContentsView::OnItemAdded(const sidebar::SidebarItem& item,
                                           int index,
                                           bool user_gesture) {
  AddItemView(item, index, user_gesture);
  InvalidateLayout();
}

void SidebarItemsContentsView::OnItemRemoved(int index) {
  RemoveChildViewT(children()[index]);
  InvalidateLayout();
}

void SidebarItemsContentsView::OnActiveIndexChanged(int old_index,
                                                    int new_index) {
  if (old_index != -1)
    UpdateItemViewStateAt(old_index, false);

  if (new_index != -1)
    UpdateItemViewStateAt(new_index, true);
}

void SidebarItemsContentsView::OnItemMoved(const sidebar::SidebarItem& item,
                                           int from,
                                           int to) {
  views::View* source_view = children()[from];
  ReorderChildView(source_view, to);
}

void SidebarItemsContentsView::AddItemView(const sidebar::SidebarItem& item,
                                           int index,
                                           bool user_gesture) {
  auto* item_view = AddChildViewAt(
      std::make_unique<SidebarItemView>(
          this, sidebar_model_->GetAllSidebarItems()[index].title),
      index);
  item_view->set_context_menu_controller(this);
  item_view->set_paint_background_on_hovered(true);
  item_view->SetCallback(
      base::BindRepeating(&SidebarItemsContentsView::OnItemPressed,
                          base::Unretained(this), item_view));
  item_view->set_drag_controller(drag_controller_);

  if (sidebar::IsWebType(item))
    SetDefaultImageAt(index, item);

  UpdateItemViewStateAt(index, false);
}

void SidebarItemsContentsView::SetDefaultImageAt(
    int index,
    const sidebar::SidebarItem& item) {
  SkColor text_color = SK_ColorWHITE;
  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    text_color = theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_BUTTON_BASE);
  }

  const int scale = GetWidget()->GetCompositor()->device_scale_factor();
  gfx::Canvas canvas(kIconSize, scale, false);

  // TODO(simonhong): Ask this design to UX team for default image generation.
  // Use bigger font(8px larger than default for test) for text favicon.
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  canvas.DrawStringRectWithFlags(
      base::i18n::ToUpper(base::UTF8ToUTF16(GetFirstCharFromURL(item.url))),
      rb.GetFontListWithDelta(8), text_color, gfx::Rect(kIconSize),
      gfx::Canvas::TEXT_ALIGN_CENTER);

  SetImageForItem(item,
                  gfx::ImageSkia(gfx::ImageSkiaRep(canvas.GetBitmap(), scale)));
}

void SidebarItemsContentsView::ShowItemAddedFeedbackBubble() {
  auto* prefs = browser_->profile()->GetPrefs();
  const int current_count =
      prefs->GetInteger(sidebar::kSidebarItemAddedFeedbackBubbleShowCount);
  // Don't show feedback bubble more than three times.
  if (current_count >= 3)
    return;
  prefs->SetInteger(sidebar::kSidebarItemAddedFeedbackBubbleShowCount,
                    current_count + 1);

  auto* lastly_added_view = children()[children().size() - 1];
  ShowItemAddedFeedbackBubble(lastly_added_view);
}

void SidebarItemsContentsView::ShowItemAddedFeedbackBubble(
    views::View* anchor_view) {
  // Only launch feedback bubble for active browser window.
  DCHECK_EQ(browser_, BrowserList::GetInstance()->GetLastActive());
  DCHECK(!observation_.IsObserving());

  auto* bubble = views::BubbleDialogDelegateView::CreateBubble(
      new SidebarItemAddedFeedbackBubble(anchor_view, this));
  observation_.Observe(bubble);
  bubble->Show();
}

bool SidebarItemsContentsView::IsBuiltInTypeItemView(views::View* view) const {
  const int index = GetIndexOf(view);
  return sidebar::IsBuiltInType(sidebar_model_->GetAllSidebarItems()[index]);
}

void SidebarItemsContentsView::SetImageForItem(const sidebar::SidebarItem& item,
                                               const gfx::ImageSkia& image) {
  int index = sidebar_model_->GetIndexOf(item);
  // -1 means |item| is deleted while fetching favicon.
  if (index == -1)
    return;

  SidebarItemView* item_view = GetItemViewAt(index);
  item_view->SetImage(
      views::Button::STATE_NORMAL,
      gfx::ImageSkiaOperations::CreateResizedImage(
          image, skia::ImageOperations::RESIZE_BEST, kIconSize));
}

void SidebarItemsContentsView::ClearDragIndicator() {
  for (auto* view : children()) {
    static_cast<SidebarItemView*>(view)->ClearHorizontalBorder();
  }
}

int SidebarItemsContentsView::CalculateTargetDragIndicatorIndex(
    const gfx::Point& screen_position) {
  // Find which item view includes this |screen_position|.
  const int child_count = children().size();
  for (int i = 0; i < child_count; ++i) {
    views::View* child_view = children()[i];
    gfx::Rect child_view_rect = child_view->GetLocalBounds();
    views::View::ConvertRectToScreen(child_view, &child_view_rect);

    if (child_view_rect.Contains(screen_position)) {
      const gfx::Point center_point = child_view_rect.CenterPoint();
      return center_point.y() > screen_position.y() ? i : i + 1;
    }
  }

  NOTREACHED();
  return -1;
}

int SidebarItemsContentsView::DrawDragIndicator(views::View* source,
                                                const gfx::Point& position) {
  const int source_view_index = GetIndexOf(source);
  int target_index = CalculateTargetDragIndicatorIndex(position);
  // If target position is right before or right after, don't need to draw
  // drag indicator.
  if (source_view_index == target_index ||
      source_view_index + 1 == target_index) {
    ClearDragIndicator();
  } else {
    DoDrawDragIndicator(target_index);
  }

  return target_index;
}

void SidebarItemsContentsView::DoDrawDragIndicator(int index) {
  // Clear current drag indicator.
  ClearDragIndicator();

  if (index == -1)
    return;

  // Use item's top or bottom border as a drag indicator.
  // Item's top border is used as a drag indicator except last item.
  // Last item's bottom border is used for indicator when drag candidate
  // position is behind the last item.
  const int child_count = children().size();
  const bool draw_top_border = child_count != index;
  const int item_index = draw_top_border ? index : index - 1;
  GetItemViewAt(item_index)->DrawHorizontalBorder(draw_top_border);
}

SidebarItemView* SidebarItemsContentsView::GetItemViewAt(int index) {
  return static_cast<SidebarItemView*>(children()[index]);
}

void SidebarItemsContentsView::UpdateItemViewStateAt(int index, bool active) {
  const auto item = sidebar_model_->GetAllSidebarItems()[index];
  SidebarItemView* item_view = GetItemViewAt(index);

  if (item.open_in_panel)
    item_view->set_draw_highlight(active);

  if (sidebar::IsBuiltInType(item)) {
    item_view->SetImage(
        views::Button::STATE_NORMAL,
        GetImageForBuiltInItems(item.built_in_item_type, active));
    item_view->SetImage(views::Button::STATE_HOVERED,
                        GetImageForBuiltInItems(item.built_in_item_type, true));
    item_view->SetImage(views::Button::STATE_PRESSED,
                        GetImageForBuiltInItems(item.built_in_item_type, true));
  }
}

void SidebarItemsContentsView::OnItemPressed(const views::View* item,
                                             const ui::Event& event) {
  auto* controller = browser_->sidebar_controller();
  const int index = GetIndexOf(item);
  if (controller->IsActiveIndex(index)) {
    // TODO(simonhong): This is for demo. We will have another UI for closing.
    // De-activate active item.
    controller->ActivateItemAt(-1);
    return;
  }

  WindowOpenDisposition open_disposition = WindowOpenDisposition::CURRENT_TAB;
  if (event_utils::IsPossibleDispositionEvent(event))
    open_disposition = ui::DispositionFromEventFlags(event.flags());

  controller->ActivateItemAt(index, open_disposition);
}

gfx::ImageSkia SidebarItemsContentsView::GetImageForBuiltInItems(
    sidebar::SidebarItem::BuiltInItemType type,
    bool focused) const {
  SkColor base_button_color = SK_ColorWHITE;
  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    base_button_color = theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_BUTTON_BASE);
  }
  constexpr int kBuiltInIconSize = 16;
  int focused_image_resource = -1;
  const gfx::VectorIcon* normal_image_icon = nullptr;
  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  switch (type) {
    case sidebar::SidebarItem::BuiltInItemType::kWallet:
      focused_image_resource = IDR_SIDEBAR_CRYPTO_WALLET_FOCUSED;
      normal_image_icon = &kSidebarCryptoWalletIcon;
      break;
    case sidebar::SidebarItem::BuiltInItemType::kBraveTalk:
      focused_image_resource = IDR_SIDEBAR_BRAVE_TALK_FOCUSED;
      normal_image_icon = &kSidebarBraveTalkIcon;
      break;
    case sidebar::SidebarItem::BuiltInItemType::kBookmarks:
      focused_image_resource = IDR_SIDEBAR_BOOKMARKS_FOCUSED;
      normal_image_icon = &kSidebarBookmarksIcon;
      break;
    case sidebar::SidebarItem::BuiltInItemType::kHistory:
      focused_image_resource = IDR_SIDEBAR_HISTORY_FOCUSED;
      normal_image_icon = &kSidebarHistoryIcon;
      break;
    default:
      NOTREACHED();
      return gfx::ImageSkia();
  }

  if (focused) {
    return gfx::ImageSkiaOperations::CreateResizedImage(
        *bundle.GetImageSkiaNamed(focused_image_resource),
        skia::ImageOperations::RESIZE_BEST,
        gfx::Size{kBuiltInIconSize, kBuiltInIconSize});
  }

  return gfx::CreateVectorIcon(*normal_image_icon, kBuiltInIconSize,
                               base_button_color);
}

void SidebarItemsContentsView::OnWidgetDestroying(views::Widget* widget) {
  observation_.Reset();
}

bool SidebarItemsContentsView::IsBubbleVisible() const {
  if (context_menu_runner_ && context_menu_runner_->IsRunning())
    return true;

  if (observation_.IsObserving())
    return true;

  return false;
}

BEGIN_METADATA(SidebarItemsContentsView, views::View)
END_METADATA
