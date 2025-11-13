/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_images_source_base.h"

#include <optional>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"

namespace brave_wallet {

BlockchainImagesSourceBase::BlockchainImagesSourceBase(
    const base::FilePath& base_path)
    : base_path_(base_path) {}

BlockchainImagesSourceBase::~BlockchainImagesSourceBase() = default;

void BlockchainImagesSourceBase::StartDataRequestForPath(
    std::string_view path,
    GotDataCallback callback) {
  std::optional<base::Version> version =
      brave_wallet::GetLastInstalledWalletVersion();
  if (!version) {
    std::move(callback).Run({});
    return;
  }

  base::FilePath images_path(base_path_);
  images_path = images_path.AppendASCII(version->GetString());
  images_path = images_path.AppendASCII("images");
  images_path = images_path.AppendASCII(path);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&base::ReadFileToBytes, images_path),
      base::BindOnce(&BlockchainImagesSourceBase::OnGotImageFileBytes,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BlockchainImagesSourceBase::OnGotImageFileBytes(
    base::OnceCallback<void(scoped_refptr<base::RefCountedMemory>)> callback,
    std::optional<std::vector<uint8_t>> input) {
  if (!input) {
    std::move(callback).Run({});
    return;
  }

  std::move(callback).Run(
      base::MakeRefCounted<base::RefCountedBytes>(std::move(*input)));
}

std::string BlockchainImagesSourceBase::GetMimeTypeForPath(
    std::string_view path) const {
  if (base::EndsWith(path, ".png", base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/png";
  }
  if (base::EndsWith(path, ".gif", base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/gif";
  }
  if (base::EndsWith(path, ".jpg", base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/jpg";
  }
  return "image/svg+xml";
}

}  // namespace brave_wallet
