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

namespace {

std::optional<std::string> ReadFileToString(const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    return std::optional<std::string>();
  }
  return contents;
}

}  // namespace

BlockchainImagesSourceBase::BlockchainImagesSourceBase(
    const base::FilePath& base_path)
    : base_path_(base_path) {}

BlockchainImagesSourceBase::~BlockchainImagesSourceBase() = default;

void BlockchainImagesSourceBase::HandleImageRequest(
    const std::string& path,
    base::OnceCallback<void(scoped_refptr<base::RefCountedMemory>)> callback) {
  std::optional<base::Version> version =
      brave_wallet::GetLastInstalledWalletVersion();
  if (!version) {
    scoped_refptr<base::RefCountedMemory> bytes;
    std::move(callback).Run(std::move(bytes));
    return;
  }

  base::FilePath images_path(base_path_);
  images_path = images_path.AppendASCII(version->GetString());
  images_path = images_path.AppendASCII("images");

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFileToString, images_path.AppendASCII(path)),
      base::BindOnce(&BlockchainImagesSourceBase::OnGotImageFile,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BlockchainImagesSourceBase::OnGotImageFile(
    base::OnceCallback<void(scoped_refptr<base::RefCountedMemory>)> callback,
    std::optional<std::string> input) {
  scoped_refptr<base::RefCountedMemory> bytes;
  if (!input) {
    std::move(callback).Run(std::move(bytes));
    return;
  }

  std::move(callback).Run(
      new base::RefCountedBytes(base::as_byte_span(*input)));
}

std::string BlockchainImagesSourceBase::GetMimeTypeForPath(
    const std::string& path) const {
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
