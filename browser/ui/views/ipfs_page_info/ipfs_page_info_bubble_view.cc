/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/ipfs_page_info/ipfs_page_info_bubble_view.h"

#include <stddef.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/i18n/rtl.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "build/build_config.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/chrome_typography.h"
#include "components/grit/brave_components_strings.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/metadata/metadata_impl_macros.h"
#include "url/gurl.h"

namespace {

const int kExclamationIconSize = 24;
const int kFirstRowHeight = 30;
const int kCommonMargins = 15;
const int kTextColumnWidth = 325;
const int kCornerRadius = 15;

// Bubble background color
const SkColor kBackgroundColor = SK_ColorWHITE;

// Adds a ColumnSet on |layout| with a single View column and padding columns
// on either side of it with |margin| width.
void AddColumnWithSideMargin(views::GridLayout* layout, int margin, int id) {
  views::ColumnSet* column_set = layout->AddColumnSet(id);
  column_set->AddPaddingColumn(views::GridLayout::kFixedSize, 0);
  column_set->AddColumn(views::GridLayout::LEADING, views::GridLayout::LEADING,
                        1.0, views::GridLayout::ColumnSize::kUsePreferred, 0,
                        0);
  column_set->AddPaddingColumn(views::GridLayout::kFixedSize, margin);
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 1.0,
                        views::GridLayout::ColumnSize::kUsePreferred, 0, 0);
  column_set->AddPaddingColumn(views::GridLayout::kFixedSize, 0);
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// IpfsPageInfoBubbleView
////////////////////////////////////////////////////////////////////////////////
IpfsPageInfoBubbleView::IpfsPageInfoBubbleView(
    views::View* anchor_view,
    const gfx::Rect& anchor_rect,
    gfx::NativeView parent_window,
    content::WebContents* web_contents,
    const GURL& url)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_LEFT),
      content::WebContentsObserver(web_contents) {
  if (!ipfs::IsIPFSScheme(url)) {
    NOTREACHED();
  }
  set_title_margins(
      ChromeLayoutProvider::Get()->GetInsetsMetric(views::INSETS_DIALOG));
  set_margins(gfx::Insets(kCommonMargins));
  set_close_on_deactivate(true);
  set_use_round_corners(true);
  SetShowCloseButton(false);
  set_color(kBackgroundColor);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_parent_window(parent_window);

  views::BubbleDialogDelegateView::CreateBubble(this);

  views::GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>());
  const int label_column_status = 1;
  AddColumnWithSideMargin(layout, kCommonMargins, label_column_status);

  layout->StartRow(views::GridLayout::kFixedSize, label_column_status,
                   kFirstRowHeight);
  AddExclamationIcon(layout);
  AddTitleLabel(layout);

  layout->StartRow(views::GridLayout::kFixedSize, label_column_status);
  FillEmptyCell(layout);
  AddBodyText(layout);

  GetBubbleFrameView()->SetCornerRadius(kCornerRadius);
  SizeToContents();
}

IpfsPageInfoBubbleView::~IpfsPageInfoBubbleView() {}

BEGIN_METADATA(IpfsPageInfoBubbleView, views::BubbleDialogDelegateView)
END_METADATA

void IpfsPageInfoBubbleView::FillEmptyCell(views::GridLayout* layout) {
  auto flex = std::make_unique<View>();
  flex->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->AddView(std::move(flex), 1.0, 1.0, views::GridLayout::CENTER,
                  views::GridLayout::CENTER);
}

void IpfsPageInfoBubbleView::AddExclamationIcon(views::GridLayout* layout) {
  gfx::ImageSkia image =
      gfx::CreateVectorIcon(kExclamationIcon, kExclamationIconSize,
                            GetNativeTheme()->GetSystemColor(
                                ui::NativeTheme::kColorId_DefaultIconColor));

  std::unique_ptr<views::ImageView> icon_view(new views::ImageView());
  icon_view->SetImage(image);
  icon_view->SetHorizontalAlignment(views::ImageView::Alignment::kLeading);
  layout->AddView(std::move(icon_view), 1.0, 1.0, views::GridLayout::CENTER,
                  views::GridLayout::CENTER);
}

void IpfsPageInfoBubbleView::AddTitleLabel(views::GridLayout* layout) {
  auto title_label = std::make_unique<views::StyledLabel>();

  const base::string16 protocol =
      l10n_util::GetStringUTF16(IDS_PAGE_INFO_IPFS_BUBBLE_TEXT_IPFS_PROTOCOL);
  size_t offset = 0;
  const base::string16 text = l10n_util::GetStringFUTF16(
      IDS_PAGE_INFO_IPFS_BUBBLE_TITTLE, protocol, &offset);
  title_label->SetText(text);

  views::StyledLabel::RangeStyleInfo title_style;
  gfx::Range title_details(0, offset);
  title_label->AddStyleRange(title_details, title_style);

  gfx::Range settings_details(offset, text.length());
  views::StyledLabel::RangeStyleInfo bold_style;
  bold_style.text_style = STYLE_EMPHASIZED_SECONDARY;
  title_label->AddStyleRange(settings_details, bold_style);

  layout->AddView(std::move(title_label), 2.0, 1.0, views::GridLayout::LEADING,
                  views::GridLayout::CENTER);
}

void IpfsPageInfoBubbleView::AddBodyText(views::GridLayout* layout) {
  std::vector<size_t> offsets;
  const base::string16 learn_more_text =
      l10n_util::GetStringUTF16(IDS_PAGE_INFO_IPFS_BUBBLE_TEXT_LEARN_MORE);
  const base::string16 settings_text =
      l10n_util::GetStringUTF16(IDS_PAGE_INFO_IPFS_BUBBLE_TEXT_IPFS_SETTINGS);
  const base::string16 explanation_text = l10n_util::GetStringFUTF16(
      IDS_PAGE_INFO_IPFS_BUBBLE_TEXT, learn_more_text, settings_text, &offsets);
  auto body_text = std::make_unique<views::StyledLabel>();
  body_text->SetText(explanation_text);
  body_text->AddStyleRange(
      gfx::Range(offsets[0], offsets[0] + learn_more_text.size()),
      views::StyledLabel::RangeStyleInfo::CreateForLink(base::BindRepeating(
          &IpfsPageInfoBubbleView::LearnMoreClicked, base::Unretained(this))));
  body_text->AddStyleRange(
      gfx::Range(offsets[1], offsets[1] + settings_text.size()),
      views::StyledLabel::RangeStyleInfo::CreateForLink(
          base::BindRepeating(&IpfsPageInfoBubbleView::SettingsLinkClicked,
                              base::Unretained(this))));
  body_text->SizeToFit(kTextColumnWidth);
  layout->AddView(std::move(body_text), 2.0, 1.0, views::GridLayout::LEADING,
                  views::GridLayout::CENTER);
}

void IpfsPageInfoBubbleView::SettingsLinkClicked(const ui::Event& event) {
  web_contents()->OpenURL(content::OpenURLParams(
      GURL(ipfs::kIPFSSettingsURL), content::Referrer(),
      ui::DispositionFromEventFlags(event.flags(),
                                    WindowOpenDisposition::NEW_FOREGROUND_TAB),
      ui::PAGE_TRANSITION_LINK, false));
}

void IpfsPageInfoBubbleView::LearnMoreClicked(const ui::Event& event) {
  web_contents()->OpenURL(content::OpenURLParams(
      GURL(ipfs::kIPFSLearnMoreURL), content::Referrer(),
      ui::DispositionFromEventFlags(event.flags(),
                                    WindowOpenDisposition::NEW_FOREGROUND_TAB),
      ui::PAGE_TRANSITION_LINK, false));
}

// static
views::BubbleDialogDelegateView* IpfsPageInfoBubbleView::CreatePageInfoBubble(
    views::View* anchor_view,
    const gfx::Rect& anchor_rect,
    gfx::NativeWindow parent_window,
    Profile* profile,
    content::WebContents* web_contents,
    const GURL& url,
    PageInfoClosingCallback closing_callback) {
  gfx::NativeView parent_view = platform_util::GetViewForWindow(parent_window);

  return new IpfsPageInfoBubbleView(anchor_view, anchor_rect, parent_view,
                                    web_contents, url);
}

// WebContentsObserver:
void IpfsPageInfoBubbleView::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host == web_contents()->GetMainFrame()) {
    GetWidget()->Close();
  }
}

void IpfsPageInfoBubbleView::OnVisibilityChanged(
    content::Visibility visibility) {
  if (visibility == content::Visibility::HIDDEN)
    GetWidget()->Close();
}

void IpfsPageInfoBubbleView::DidStartNavigation(
    content::NavigationHandle* handle) {
  if (handle->IsInMainFrame())
    GetWidget()->Close();
}

void IpfsPageInfoBubbleView::DidChangeVisibleSecurityState() {
  // Subclasses may update instead, but this the only safe general option.
  GetWidget()->Close();
}
