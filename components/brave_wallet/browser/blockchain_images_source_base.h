/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_IMAGES_SOURCE_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_IMAGES_SOURCE_BASE_H_

#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"

namespace base {
class FilePath;
}  // namespace base

namespace brave_wallet {

// This serves background image data.
class BlockchainImagesSourceBase {
 protected:
  explicit BlockchainImagesSourceBase(const base::FilePath& base_path);

  virtual ~BlockchainImagesSourceBase();

  BlockchainImagesSourceBase(const BlockchainImagesSourceBase&) = delete;
  BlockchainImagesSourceBase& operator=(const BlockchainImagesSourceBase&) =
      delete;

  using GotDataCallback =
      base::OnceCallback<void(scoped_refptr<base::RefCountedMemory>)>;
  void StartDataRequestForPath(std::string_view path, GotDataCallback callback);
  std::string GetMimeTypeForPath(std::string_view path) const;
  bool AllowCaching() const { return true; }
  void OnGotImageFileBytes(
      base::OnceCallback<void(scoped_refptr<base::RefCountedMemory>)> callback,
      std::optional<std::vector<uint8_t>> input);

  base::FilePath base_path_;
  base::WeakPtrFactory<BlockchainImagesSourceBase> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_IMAGES_SOURCE_BASE_H_
