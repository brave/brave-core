/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/erc_token_images_source.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace brave_wallet {

namespace {

absl::optional<std::string> ReadFileToString(const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents))
    return absl::optional<std::string>();
  return contents;
}

}  // namespace

ERCTokenImagesSource::ERCTokenImagesSource(const base::FilePath& base_path)
    : base_path_(base_path), weak_factory_(this) {}

ERCTokenImagesSource::~ERCTokenImagesSource() = default;

std::string ERCTokenImagesSource::GetSource() {
  return kImageSourceHost;
}

void ERCTokenImagesSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const std::string path = URLDataSource::URLToRequestPath(url);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFileToString, base_path_.AppendASCII(path)),
      base::BindOnce(&ERCTokenImagesSource::OnGotImageFile,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ERCTokenImagesSource::OnGotImageFile(GotDataCallback callback,
                                          absl::optional<std::string> input) {
  scoped_refptr<base::RefCountedMemory> bytes;
  if (!input) {
    std::move(callback).Run(std::move(bytes));
    return;
  }

  bytes = new base::RefCountedBytes(
      reinterpret_cast<const unsigned char*>(input->c_str()), input->length());
  std::move(callback).Run(std::move(bytes));
}

std::string ERCTokenImagesSource::GetMimeType(const std::string& path) {
  if (base::EndsWith(path, ".png", base::CompareCase::INSENSITIVE_ASCII))
    return "image/png";
  if (base::EndsWith(path, ".gif", base::CompareCase::INSENSITIVE_ASCII))
    return "image/gif";
  if (base::EndsWith(path, ".jpg", base::CompareCase::INSENSITIVE_ASCII))
    return "image/jpg";
  return "image/svg+xml";
}

bool ERCTokenImagesSource::AllowCaching() {
  return true;
}

}  // namespace brave_wallet
