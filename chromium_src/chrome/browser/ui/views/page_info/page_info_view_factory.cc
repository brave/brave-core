/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/page_info/page_info_view_factory.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/vector_icons/vector_icons.h"
#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/ui/page_info/chrome_page_info_ui_delegate.h"
#include "chrome/browser/ui/views/page_info/page_info_hover_button.h"
#include "components/grit/brave_components_strings.h"

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
      brave_l10n::GetLocalizedResourceUTF16String(tooltip_resource_id);

  return std::make_unique<PageInfoHoverButton>(
      std::move(callback), ui::ImageModel::FromImageSkia(ipfs_logo),
      text_resource_id, std::u16string(),
      PageInfoViewFactory::VIEW_ID_PAGE_INFO_LINK_OR_BUTTON_COOKIE_DIALOG,
      tooltip, std::u16string());
}

void BraveAddIPFSButtons(views::View* container,
                         ChromePageInfoUiDelegate* delegate) {
  if (!container || !delegate)
    return;
  container->AddChildView(CreateButton(
      IDR_BRAVE_IPFS_LOGO, IDS_PAGE_INFO_IPFS_SETTINGS_BUTTON_TEXT,
      IDS_PAGE_INFO_IPFS_SETTINGS_BUTTON_TOOLTIP_TEXT,
      base::BindRepeating(
          [](ChromePageInfoUiDelegate* ui_delegate, const ui::Event& event) {
            ui_delegate->AddIPFSTabForURL(GURL(ipfs::kIPFSSettingsURL));
          },
          delegate)));
  container->AddChildView(CreateButton(
      IDR_BRAVE_IPFS_LOGO, IDS_PAGE_INFO_IPFS_DIAGNOSTICS_BUTTON_TEXT,
      IDS_PAGE_INFO_IPFS_DIAGNOSTICS_BUTTON_TOOLTIP_TEXT,
      base::BindRepeating(
          [](ChromePageInfoUiDelegate* ui_delegate, const ui::Event& event) {
            ui_delegate->AddIPFSTabForURL(GURL(kIPFSWebUIURL));
          },
          delegate)));
  container->AddChildView(CreateButton(
      IDR_BRAVE_IPFS_LOGO, IDS_PAGE_INFO_IPFS_DOCS_BUTTON_TEXT,
      IDS_PAGE_INFO_IPFS_DOCS_BUTTON_TEXT_TOOLTIP_TEXT,
      base::BindRepeating(
          [](ChromePageInfoUiDelegate* ui_delegate, const ui::Event& event) {
            ui_delegate->AddIPFSTabForURL(GURL(kIPFSDocsURL));
          },
          delegate)));
}

}  // namespace

#define CreateSecurityPageView CreateSecurityPageView_ChromiumImpl
#endif  // BUILDFLAG(ENABLE_IPFS)

#define BRAVE_PAGE_INFO_VIEW_FACTORY_GET_PERMISSION_ICON \
  case ContentSettingsType::AUTOPLAY:                    \
    icon = &kAutoplayStatusIcon;                         \
    break;

#include "src/chrome/browser/ui/views/page_info/page_info_view_factory.cc"

#undef BRAVE_PAGE_INFO_VIEW_FACTORY_GET_PERMISSION_ICON

#if BUILDFLAG(ENABLE_IPFS)
#undef CreateSecurityPageView

std::unique_ptr<views::View> PageInfoViewFactory::CreateSecurityPageView() {
  auto page_view = CreateSecurityPageView_ChromiumImpl();
  if (!ipfs::IsIPFSScheme(presenter_->site_url()))
    return page_view;
  BraveAddIPFSButtons(page_view.get(), ui_delegate_);
  return page_view;
}
#endif  // BUILDFLAG(ENABLE_IPFS)
