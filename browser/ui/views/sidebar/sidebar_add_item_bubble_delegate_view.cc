/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_add_item_bubble_delegate_view.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/sidebar/bubble_border_with_arrow.h"
#include "brave/browser/ui/views/sidebar/sidebar_bubble_background.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view_class_properties.h"
#include "url/gurl.h"

namespace {

constexpr gfx::Size kAddItemBubbleEntrySize{242, 40};

sidebar::SidebarService* GetSidebarService(Browser* browser) {
  return sidebar::SidebarServiceFactory::GetForProfile(browser->profile());
}

class SidebarAddItemButton : public views::LabelButton {
 public:
  // Get theme provider to use browser's theme color in this dialog.
  SidebarAddItemButton(bool bold, const ui::ThemeProvider* theme_provider)
      : theme_provider_(theme_provider) {
    constexpr gfx::Insets kDefaultAddItemInsets{10, 34, 4, 8};
    SetBorder(views::CreateEmptyBorder(kDefaultAddItemInsets));
    if (theme_provider_) {
      SetTextColor(
          views::Button::STATE_NORMAL,
          theme_provider->GetColor(
              BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_NORMAL));
      SetTextColor(views::Button::STATE_HOVERED,
                   theme_provider->GetColor(
                       BraveThemeProperties::
                           COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_HOVERED));
      SetTextColor(views::Button::STATE_PRESSED,
                   theme_provider->GetColor(
                       BraveThemeProperties::
                           COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_HOVERED));
    }

    const int size_diff = 13 - views::Label::GetDefaultFontList().GetFontSize();
    label()->SetFontList(
        views::Label::GetDefaultFontList()
            .DeriveWithSizeDelta(size_diff)
            .DeriveWithWeight(bold ? gfx::Font::Weight::SEMIBOLD
                                   : gfx::Font::Weight::NORMAL));
  }

  gfx::Size CalculatePreferredSize() const override {
    return kAddItemBubbleEntrySize;
  }

  void OnPaintBackground(gfx::Canvas* canvas) override {
    if (GetState() == STATE_HOVERED) {
      cc::PaintFlags flags;
      flags.setAntiAlias(true);
      flags.setStyle(cc::PaintFlags::kFill_Style);
      if (theme_provider_) {
        flags.setColor(theme_provider_->GetColor(
            BraveThemeProperties::
                COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_BACKGROUND_HOVERED));
      }

      constexpr int kItemRadius = 6;
      // Fill the background.
      canvas->DrawRoundRect(GetLocalBounds(), kItemRadius, flags);
    }
  }

 private:
  const ui::ThemeProvider* theme_provider_;
};

}  // namespace

SidebarAddItemBubbleDelegateView::SidebarAddItemBubbleDelegateView(
    BraveBrowser* browser,
    views::View* anchor_view)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::LEFT_TOP),
      browser_(browser) {
  DCHECK(browser_);
  // Give margin and arrow at there.
  set_margins(
      gfx::Insets(0, BubbleBorderWithArrow::kBubbleArrowBoundsWidth, 0, 0));
  set_title_margins(gfx::Insets());
  SetButtons(ui::DIALOG_BUTTON_NONE);

  if (auto* theme_provider =
          BrowserView::GetBrowserViewForBrowser(browser_)->GetThemeProvider()) {
    set_color(theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_BACKGROUND));
  }
  AddChildViews();
}

SidebarAddItemBubbleDelegateView::~SidebarAddItemBubbleDelegateView() = default;

std::unique_ptr<views::NonClientFrameView>
SidebarAddItemBubbleDelegateView::CreateNonClientFrameView(
    views::Widget* widget) {
  std::unique_ptr<views::BubbleFrameView> frame(
      new views::BubbleFrameView(gfx::Insets(), gfx::Insets()));
  std::unique_ptr<BubbleBorderWithArrow> border =
      std::make_unique<BubbleBorderWithArrow>(arrow(), GetShadow(), color());
  constexpr int kRadius = 4;
  border->SetCornerRadius(kRadius);
  auto* border_ptr = border.get();
  frame->SetBubbleBorder(std::move(border));
  // Replace frame's background to draw arrow.
  frame->SetBackground(std::make_unique<SidebarBubbleBackground>(border_ptr));
  return frame;
}

void SidebarAddItemBubbleDelegateView::AddChildViews() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  // |site_part| includes Add item header and current tab url.
  views::View* site_part = AddChildView(std::make_unique<views::View>());
  site_part->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(4, 4, 6, 4), 6));
  // Use 13px font size.
  const int size_diff = 13 - views::Label::GetDefaultFontList().GetFontSize();
  views::Label::CustomFont font = {
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD)};
  auto* header = site_part->AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_SIDEBAR_ADD_ITEM_BUBBLE_TITLE), font));
  auto* theme_provider =
      BrowserView::GetBrowserViewForBrowser(browser_)->GetThemeProvider();
  if (theme_provider) {
    header->SetEnabledColor(theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_HEADER_TEXT));
  }
  header->SetAutoColorReadabilityEnabled(false);
  header->SetPreferredSize(kAddItemBubbleEntrySize);
  constexpr gfx::Insets kHeaderInsets{10, 34, 4, 8};
  header->SetBorder(views::CreateEmptyBorder(kHeaderInsets));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  if (sidebar::CanAddCurrentActiveTabToSidebar(browser_)) {
    auto* button = site_part->AddChildView(
        std::make_unique<SidebarAddItemButton>(true, theme_provider));
    const GURL active_tab_url =
        browser_->tab_strip_model()->GetActiveWebContents()->GetVisibleURL();
    button->SetText(base::UTF8ToUTF16(active_tab_url.host()));
    button->SetCallback(base::BindRepeating(
        &SidebarAddItemBubbleDelegateView::OnCurrentItemButtonPressed,
        base::Unretained(this)));
  }

  const auto not_added_default_items =
      GetSidebarService(browser_)->GetNotAddedDefaultSidebarItems();
  if (not_added_default_items.empty())
    return;

  auto* separator = AddChildView(std::make_unique<views::Separator>());
  if (theme_provider) {
    separator->SetColor(theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_SEPARATOR));
  }

  // |default_part| includes not added default items.
  views::View* default_part = AddChildView(std::make_unique<views::View>());
  default_part->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(6, 4, 8, 4), 8));

  for (const auto& item : not_added_default_items) {
    auto* button = default_part->AddChildView(
        std::make_unique<SidebarAddItemButton>(false, theme_provider));
    button->SetText(item.title);
    button->SetCallback(base::BindRepeating(
        &SidebarAddItemBubbleDelegateView::OnDefaultItemsButtonPressed,
        base::Unretained(this), item));
  }
}

void SidebarAddItemBubbleDelegateView::OnDefaultItemsButtonPressed(
    const sidebar::SidebarItem& item) {
  GetSidebarService(browser_)->AddItem(item);
  RemoveAllChildViews(true);
  AddChildViews();
  if (children().size() == 1)
    GetWidget()->CloseWithReason(views::Widget::ClosedReason::kUnspecified);
}

void SidebarAddItemBubbleDelegateView::OnCurrentItemButtonPressed() {
  browser_->sidebar_controller()->AddItemWithCurrentTab();
  RemoveAllChildViews(true);
  AddChildViews();
  if (children().size() == 1)
    GetWidget()->CloseWithReason(views::Widget::ClosedReason::kUnspecified);
}
