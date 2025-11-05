/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/blockchain_images_source.h"

#include <optional>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/blockchain_images_source_base.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "content/public/browser/browser_thread.h"

namespace brave_wallet {

BlockchainImagesSource::BlockchainImagesSource(const base::FilePath& base_path)
    : BlockchainImagesSourceBase(base_path) {}

std::string BlockchainImagesSource::GetSource() {
  return kImageSourceHost;
}

void BlockchainImagesSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const std::string path = URLDataSource::URLToRequestPath(url);
  HandleImageRequest(path, std::move(callback));
}

std::string BlockchainImagesSource::GetMimeType(const GURL& url) {
  return GetMimeTypeForPath(URLToRequestPath(url));
}

bool BlockchainImagesSource::AllowCaching() {
  return true;
}

}  // namespace brave_wallet
