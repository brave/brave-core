// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_wallet/blockchain_images_source.h"

#include <optional>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/blockchain_images_source_base.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"

namespace brave_wallet {

BlockchainImagesSource::BlockchainImagesSource(const base::FilePath& base_path)
    : BlockchainImagesSourceBase(base_path) {}

std::string BlockchainImagesSource::GetSource() const {
  return kImageSourceHost;
}

void BlockchainImagesSource::StartDataRequest(const std::string& path,
                                              GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  HandleImageRequest(path, std::move(callback));
}

std::string BlockchainImagesSource::GetMimeType(const std::string& path) const {
  return GetMimeTypeForPath(path);
}

bool BlockchainImagesSource::AllowCaching() const {
  return BlockchainImagesSourceBase::AllowCaching();
}

}  // namespace brave_wallet
