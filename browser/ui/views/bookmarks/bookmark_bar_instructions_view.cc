/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/bookmarks/bookmark_bar_instructions_view.h"

#include <algorithm>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/brave_view_ids.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/color/color_palette.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"

namespace {

// The labels here aren't really button labels, but we use CONTEXT_BUTTON to
// match the style of the LabelButton text normally used in bookmarks bar.
constexpr int kBookmarkBarTextContext = views::style::CONTEXT_BUTTON;

// Horizontal padding, in pixels, between the link and label.
int GetViewPadding() {
  static int space_width = views::Label(u" ").GetPreferredSize().width();
  return space_width;
}

}  // namespace

BookmarkBarInstructionsView::BookmarkBarInstructionsView(Browser* browser)
    : browser_(browser) {
  SetID(BRAVE_VIEW_ID_BOOKMARK_IMPORT_INSTRUCTION_VIEW);
  instructions_ = new views::Label(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_BOOKMARKS_NO_ITEMS),
      kBookmarkBarTextContext);
  instructions_->SetAutoColorReadabilityEnabled(false);
  instructions_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  AddChildView(instructions_.get());

  if (browser_defaults::kShowImportOnBookmarkBar) {
    import_link_ = new views::Link(brave_l10n::GetLocalizedResourceUTF16String(
                                       IDS_BOOKMARK_BAR_IMPORT_LINK),
                                   kBookmarkBarTextContext);
    // We don't want the link to alter tab navigation.
    import_link_->SetCallback(
        base::BindRepeating(&BookmarkBarInstructionsView::LinkClicked,
                            base::Unretained(this)));
    import_link_->SetFocusBehavior(FocusBehavior::NEVER);
    import_link_->set_context_menu_controller(this);
    import_link_->SetAutoColorReadabilityEnabled(false);
    import_link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    AddChildView(import_link_.get());
  }
}

gfx::Size BookmarkBarInstructionsView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int height = 0, width = 0;
  for (views::View* view : children()) {
    gfx::Size pref = view->GetPreferredSize();
    height = std::max(pref.height(), height);
    width += pref.width();
  }
  width += (children().size() - 1) * GetViewPadding();
  gfx::Insets insets = GetInsets();
  return gfx::Size(width + insets.width(), height + insets.height());
}

void BookmarkBarInstructionsView::Layout(PassKey) {
  int remaining_width = width();
  int x = 0;
  for (views::View* view : children()) {
    gfx::Size pref = view->GetPreferredSize();
    int view_width = std::min(remaining_width, pref.width());
    view->SetBounds(x, 0, view_width, height());
    x += view_width + GetViewPadding();
    remaining_width = std::max(0, width() - x);
  }
}

void BookmarkBarInstructionsView::OnThemeChanged() {
  View::OnThemeChanged();
  UpdateColors();
}

void BookmarkBarInstructionsView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  instructions_->GetAccessibleNodeData(node_data);
}

void BookmarkBarInstructionsView::LinkClicked() {
  chrome::ShowImportDialog(browser_);
}

void BookmarkBarInstructionsView::ShowContextMenuForViewImpl(
    views::View* source,
    const gfx::Point& point,
    ui::mojom::MenuSourceType source_type) {
  // Do nothing here, we don't want to show the Bookmarks context menu when
  // the user right clicks on the "Import bookmarks now" link.
}

SkColor BookmarkBarInstructionsView::GetInstructionsTextColor() {
  if (const ui::ColorProvider* color_provider = GetColorProvider())
    return color_provider->GetColor(kColorBookmarkBarInstructionsText);

  return gfx::kPlaceholderColor;
}

void BookmarkBarInstructionsView::UpdateColors() {
  instructions_->SetEnabledColor(GetInstructionsTextColor());

  const ui::ColorProvider* cp = GetColorProvider();
  if (cp && import_link_) {
    import_link_->SetEnabledColor(
        cp->GetColor(kColorBookmarkBarInstructionsLink));
  }
}

BEGIN_METADATA(BookmarkBarInstructionsView)
END_METADATA
