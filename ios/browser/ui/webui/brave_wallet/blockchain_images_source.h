// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_BLOCKCHAIN_IMAGES_SOURCE_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_BLOCKCHAIN_IMAGES_SOURCE_H_

#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "ios/web/public/webui/url_data_source_ios.h"

namespace brave_wallet {

// This serves background image data.
class BlockchainImagesSource : public web::URLDataSourceIOS {
 public:
  explicit BlockchainImagesSource(const base::FilePath& base_path);

  ~BlockchainImagesSource() override;

  BlockchainImagesSource(const BlockchainImagesSource&) = delete;
  BlockchainImagesSource& operator=(const BlockchainImagesSource&) = delete;

  // web::URLDataSourceIOS overrides:
  std::string GetSource() const override;
  void StartDataRequest(const std::string& path,
                        GotDataCallback callback) override;
  std::string GetMimeType(const std::string& path) const override;
  bool AllowCaching() const override;

  void OnGotImageFile(GotDataCallback callback,
                      std::optional<std::string> input);

  base::FilePath base_path_;
  base::WeakPtrFactory<BlockchainImagesSource> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_BLOCKCHAIN_IMAGES_SOURCE_H_
