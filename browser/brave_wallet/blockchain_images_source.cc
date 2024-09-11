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
#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

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

BlockchainImagesSource::BlockchainImagesSource(const base::FilePath& base_path)
    : base_path_(base_path), weak_factory_(this) {}

BlockchainImagesSource::~BlockchainImagesSource() = default;

std::string BlockchainImagesSource::GetSource() {
  return kImageSourceHost;
}

void BlockchainImagesSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const std::string path = URLDataSource::URLToRequestPath(url);

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
      base::BindOnce(&BlockchainImagesSource::OnGotImageFile,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BlockchainImagesSource::OnGotImageFile(GotDataCallback callback,
                                            std::optional<std::string> input) {
  scoped_refptr<base::RefCountedMemory> bytes;
  if (!input) {
    std::move(callback).Run(std::move(bytes));
    return;
  }

  std::move(callback).Run(
      new base::RefCountedBytes(base::as_byte_span(*input)));
}

std::string BlockchainImagesSource::GetMimeType(const GURL& url) {
  const std::string path = URLDataSource::URLToRequestPath(url);
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

bool BlockchainImagesSource::AllowCaching() {
  return true;
}

}  // namespace brave_wallet
