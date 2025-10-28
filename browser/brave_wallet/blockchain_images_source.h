/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BLOCKCHAIN_IMAGES_SOURCE_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BLOCKCHAIN_IMAGES_SOURCE_H_

#include <optional>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/blockchain_images_source_base.h"
#include "content/public/browser/url_data_source.h"

namespace base {
class FilePath;
}  // namespace base

namespace brave_wallet {

// This serves background image data.
class BlockchainImagesSource : public content::URLDataSource,
                               public BlockchainImagesSourceBase {
 public:
  explicit BlockchainImagesSource(const base::FilePath& base_path);

  // content::URLDataSource overrides:
  std::string GetSource() override;
  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override;
  std::string GetMimeType(const GURL& url) override;
  bool AllowCaching() override;

  void OnGotImageFile(GotDataCallback callback,
                      std::optional<std::string> input);
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BLOCKCHAIN_IMAGES_SOURCE_H_
