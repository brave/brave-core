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
#include "brave/browser/ui/views/sidebar/sidebar_item_added_feedback_bubble.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/ui/browser_list.h"
#include "ui/base/default_style.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/menu/menu_runner.h"
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

}  // namespace

SidebarItemsContentsView::SidebarItemsContentsView(BraveBrowser* browser)
    : browser_(browser),
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
  return {child_size.width() + GetInsets().width(),
          children().size() * child_size.height() + GetInsets().height()};
}

void SidebarItemsContentsView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateAllBuiltInItemsViewState();
}

void SidebarItemsContentsView::UpdateAllBuiltInItemsViewState() {
  const auto items = sidebar_model_->GetAllSidebarItems();
  // It's not initialized yet if child view count and items size are different.
  if (children().size() != items.size())
    return;

  // BuiltIn items different colored images depends on theme.
  const int active_index = sidebar_model_->active_index();
  int index = 0;
  for (const auto& item : items) {
    if (sidebar::IsBuiltInType(item))
      UpdateItemViewStateAt(index, index == active_index);
    index++;
  }
}

base::string16 SidebarItemsContentsView::GetTooltipTextFor(
    const views::View* view) const {
  int index = GetIndexOf(view);
  DCHECK_GE(index, 0);

  if (index == -1)
    return base::string16();

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
      l10n_util::GetStringUTF16(IDS_SIDEBAR_ITEM_CONTEXT_MENU_REMOVE),
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
    browser_->sidebar_controller()->RemoveItemAt(index);
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
  RemoveChildView(children()[index]);
  InvalidateLayout();
}

void SidebarItemsContentsView::OnActiveIndexChanged(int old_index,
                                                    int new_index) {
  if (old_index != -1)
    UpdateItemViewStateAt(old_index, false);

  if (new_index != -1)
    UpdateItemViewStateAt(new_index, true);
}

void SidebarItemsContentsView::AddItemView(const sidebar::SidebarItem& item,
                                           int index,
                                           bool user_gesture) {
  auto* item_view =
      AddChildViewAt(std::make_unique<SidebarItemView>(this), index);
  item_view->set_context_menu_controller(this);
  item_view->set_paint_background_on_hovered(true);
  item_view->SetCallback(
      base::BindRepeating(&SidebarItemsContentsView::OnItemPressed,
                          base::Unretained(this), item_view));

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
  auto* lastly_added_view = children()[children().size() - 1];
  ShowItemAddedFeedbackBubble(lastly_added_view);
}

void SidebarItemsContentsView::ShowItemAddedFeedbackBubble(
    views::View* anchor_view) {
  // Only launch feedback bubble for active browser window.
  DCHECK_EQ(browser_, BrowserList::GetInstance()->GetLastActive());

  auto* bubble = views::BubbleDialogDelegateView::CreateBubble(
      new SidebarItemAddedFeedbackBubble(anchor_view, this));
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

  SidebarItemView* item_view = static_cast<SidebarItemView*>(children()[index]);
  item_view->SetImage(
      views::Button::STATE_NORMAL,
      gfx::ImageSkiaOperations::CreateResizedImage(
          image, skia::ImageOperations::RESIZE_BEST, kIconSize));
}

void SidebarItemsContentsView::UpdateItemViewStateAt(int index, bool active) {
  const auto item = sidebar_model_->GetAllSidebarItems()[index];
  SidebarItemView* item_view = static_cast<SidebarItemView*>(children()[index]);

  if (item.open_in_panel)
    item_view->set_draw_highlight(active);

  if (sidebar::IsBuiltInType(item)) {
    const GURL& url = item.url;
    item_view->SetImage(views::Button::STATE_NORMAL,
                        GetImageForBuiltInItems(url, active));
    item_view->SetImage(views::Button::STATE_HOVERED,
                        GetImageForBuiltInItems(url, true));
    item_view->SetImage(views::Button::STATE_PRESSED,
                        GetImageForBuiltInItems(url, true));
  }
}

void SidebarItemsContentsView::OnItemPressed(const views::View* item) {
  auto* controller = browser_->sidebar_controller();
  const int index = GetIndexOf(item);
  if (controller->IsActiveIndex(index)) {
    // TODO(simonhong): This is for demo. We will have another UI for closing.
    // De-activate active item.
    controller->ActivateItemAt(-1);
    return;
  }

  controller->ActivateItemAt(index);
}

gfx::ImageSkia SidebarItemsContentsView::GetImageForBuiltInItems(
    const GURL& item_url,
    bool focused) const {
  SkColor base_button_color = SK_ColorWHITE;
  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    base_button_color = theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_BUTTON_BASE);
  }
  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  if (item_url == GURL("chrome://wallet/")) {
    if (focused)
      return *bundle.GetImageSkiaNamed(IDR_SIDEBAR_CRYPTO_WALLET_FOCUSED);
    return gfx::CreateVectorIcon(kSidebarCryptoWalletIcon, base_button_color);
  }

  if (item_url == GURL("https://together.brave.com/")) {
    if (focused)
      return *bundle.GetImageSkiaNamed(IDR_SIDEBAR_BRAVE_TOGETHER_FOCUSED);
    return gfx::CreateVectorIcon(kSidebarBraveTogetherIcon, base_button_color);
  }

  if (item_url == GURL("chrome://bookmarks/")) {
    if (focused)
      return *bundle.GetImageSkiaNamed(IDR_SIDEBAR_BOOKMARKS_FOCUSED);
    return gfx::CreateVectorIcon(kSidebarBookmarksIcon, base_button_color);
  }

  if (item_url == GURL("chrome://history/")) {
    if (focused)
      return *bundle.GetImageSkiaNamed(IDR_SIDEBAR_HISTORY_FOCUSED);
    return gfx::CreateVectorIcon(kSidebarHistoryIcon, base_button_color);
  }

  NOTREACHED();
  return gfx::ImageSkia();
}
