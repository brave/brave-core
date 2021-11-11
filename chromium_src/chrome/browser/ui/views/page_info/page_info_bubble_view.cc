/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/ui/views/page_info/page_info_view_factory.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

const char kIPFSDocsURL[] = "https://docs.ipfs.io/";

std::unique_ptr<PageInfoHoverButton> CreateButton(
    int logo_resource_id,
    int text_resource_id,
    int tooltip_resource_id,
    views::Button::PressedCallback callback) {
  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  const auto& ipfs_logo = *bundle.GetImageSkiaNamed(logo_resource_id);
  const std::u16string& tooltip =
      l10n_util::GetStringUTF16(tooltip_resource_id);

  return std::make_unique<PageInfoHoverButton>(
      std::move(callback), ui::ImageModel::FromImageSkia(ipfs_logo),
      text_resource_id, std::u16string(),
      PageInfoViewFactory::VIEW_ID_PAGE_INFO_LINK_OR_BUTTON_COOKIE_DIALOG,
      tooltip, std::u16string());
}

}  // namespace

#define BRAVE_PAGE_INFO_BUBBLE_VIEW_PAGE_INFO_BUBBLE_VIEW \
  if (ipfs::IsIPFSScheme(url))                            \
    BraveAddIPFSButtons();

#include "../../../../../../../chrome/browser/ui/views/page_info/page_info_bubble_view.cc"
#undef BRAVE_PAGE_INFO_BUBBLE_VIEW_PAGE_INFO_BUBBLE_VIEW

void PageInfoBubbleView::BraveAddIPFSButtons() {
  site_settings_view_->AddChildViewAt(
      CreateButton(IDR_BRAVE_IPFS_LOGO, IDS_PAGE_INFO_IPFS_SETTINGS_BUTTON_TEXT,
                   IDS_PAGE_INFO_IPFS_SETTINGS_BUTTON_TOOLTIP_TEXT,
                   base::BindRepeating(
                       [](PageInfoBubbleView* view) {
                         view->ui_delegate_->AddIPFSTabForURL(
                             GURL(ipfs::kIPFSSettingsURL));
                       },
                       base::Unretained(this))),
      0);
  site_settings_view_->AddChildViewAt(
      CreateButton(
          IDR_BRAVE_IPFS_LOGO, IDS_PAGE_INFO_IPFS_DIAGNOSTICS_BUTTON_TEXT,
          IDS_PAGE_INFO_IPFS_DIAGNOSTICS_BUTTON_TOOLTIP_TEXT,
          base::BindRepeating(
              [](PageInfoBubbleView* view) {
                view->ui_delegate_->AddIPFSTabForURL(GURL(kIPFSWebUIURL));
              },
              base::Unretained(this))),
      0);
  site_settings_view_->AddChildViewAt(
      CreateButton(IDR_BRAVE_IPFS_LOGO, IDS_PAGE_INFO_IPFS_DOCS_BUTTON_TEXT,
                   IDS_PAGE_INFO_IPFS_DOCS_BUTTON_TEXT_TOOLTIP_TEXT,
                   base::BindRepeating(
                       [](PageInfoBubbleView* view) {
                         view->ui_delegate_->AddIPFSTabForURL(
                             GURL(kIPFSDocsURL));
                       },
                       base::Unretained(this))),
      0);
}
