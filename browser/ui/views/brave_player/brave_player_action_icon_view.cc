/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_player/brave_player_action_icon_view.h"

#include "base/strings/strcat.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/vector_icon_types.h"

namespace {

GURL GetPlayerURL(content::WebContents* web_contents) {
  if (!web_contents) {
    return {};
  }

  GURL url = web_contents->GetLastCommittedURL();
  if (url.DomainIs("youtube.com") && url.has_path() && url.path() == "/watch" &&
      url.has_query()) {
    base::StringPairs key_value_pairs;
    base::SplitStringIntoKeyValuePairs(url.query(), '=', '&', &key_value_pairs);
    if (auto iter = base::ranges::find(key_value_pairs, "v",
                                       &base::StringPairs::value_type::first);
        iter != key_value_pairs.end()) {
      return GURL(base::StrCat({kBravePlayerURL, "youtube/", iter->second}));
    }
  }
  return {};
}

}  // namespace

BravePlayerActionIconView::BravePlayerActionIconView(
    CommandUpdater* command_updater,
    Browser* browser,
    IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
    PageActionIconView::Delegate* page_action_icon_delegate)
    : PageActionIconView(command_updater,
                         0,
                         icon_label_bubble_delegate,
                         page_action_icon_delegate,
                         "BravePlayerActionIconView",
                         /*ephemeral=*/false),
      browser_(browser) {
  SetVisible(false);

  SetLabel(l10n_util::GetStringUTF16(IDS_BRAVE_PLAYER_ACTION_VIEW));
}

BravePlayerActionIconView::~BravePlayerActionIconView() = default;

void BravePlayerActionIconView::OnExecuting(ExecuteSource execute_source) {
  CHECK(player_url_.is_valid());
  chrome::AddTabAt(browser_, player_url_, /*index*/ -1,
                   /*foreground=*/true);
}

views::BubbleDialogDelegate* BravePlayerActionIconView::GetBubble() const {
  return nullptr;
}

void BravePlayerActionIconView::UpdateIconImage() {
  SetImageModel(
      ui::ImageModel::FromResourceId(IDR_BRAVE_PLAYER_ACTION_VIEW_ICON));
}

const gfx::VectorIcon& BravePlayerActionIconView::GetVectorIcon() const {
  // We don't use vector icon because we need gradation effect.
  // TODO(sko) When Nala icon updates, try use vector icon and blending effect
  // to generate gradation effect.
  NOTREACHED_NORETURN();
}

void BravePlayerActionIconView::UpdateImpl() {
  player_url_ = GetPlayerURL(GetWebContents());
  SetVisible(player_url_.is_valid());
}

void BravePlayerActionIconView::UpdateBorder() {
  // Update insets using base class implementation.
  PageActionIconView::UpdateBorder();

  SetBorder(views::CreatePaddedBorder(
      views::CreateThemedRoundedRectBorder(
          /*thickness*/ 1,
          /*corner_radius=*/8, kColorBravePlayerActionViewBorder),
      GetInsets()));
}

BEGIN_METADATA(BravePlayerActionIconView, PageActionIconView);
END_METADATA
