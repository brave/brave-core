/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/ui/views/page_info/page_info_view_factory.h"
#include "components/page_info/page_info.h"
#include "ui/base/models/image_model.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

bool IsIPFSPage(PageInfo* presenter) {
  return presenter && ipfs::IsIPFSScheme(presenter->site_url());
}

const ui::ImageModel GetIpfsGetConnectionSecureIcon() {
  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  const auto& ipfs_logo = *bundle.GetImageSkiaNamed(IDR_BRAVE_IPFS_LOGO);
  return ui::ImageModel::FromImageSkia(ipfs_logo);
}

}  // namespace

#define GetConnectionSecureIcon                                           \
  GetConnectionSecureIcon() != ui::ImageModel() && IsIPFSPage(presenter_) \
      ? GetIpfsGetConnectionSecureIcon()                                  \
      : PageInfoViewFactory::GetConnectionSecureIcon
#define BRAVE_PAGE_INFO_MAIN_VIEW_CALCULATE_PREFERRED_SIZE                     \
  if (IsIPFSPage(presenter_)) {                                                \
    width =                                                                    \
        std::max(security_container_view_->GetPreferredSize().width(), width); \
  }

#include "../../../../../../../chrome/browser/ui/views/page_info/page_info_main_view.cc"

#undef BRAVE_PAGE_INFO_MAIN_VIEW_CALCULATE_PREFERRED_SIZE
#undef GetConnectionSecureIcon
