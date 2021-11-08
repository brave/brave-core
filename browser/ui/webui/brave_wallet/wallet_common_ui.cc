/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/wallet_common_ui.h"

#include <memory>

#include "base/version.h"
#include "brave/browser/brave_wallet/erc_token_images_source.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_wallet {

void AddERCTokenImageSource(Profile* profile) {
  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));

  base::FilePath path = profile->GetPath().DirName();
  path = path.AppendASCII(brave_wallet::kWalletBaseDirectory);
  content::URLDataSource::Add(
      profile, std::make_unique<brave_wallet::ERCTokenImagesSource>(path));
}

bool IsBraveWalletOrigin(const url::Origin& origin) {
  return origin == url::Origin::Create(GURL(kBraveUIWalletPanelURL)) ||
         origin == url::Origin::Create(GURL(kBraveUIWalletPageURL));
}

}  // namespace brave_wallet
