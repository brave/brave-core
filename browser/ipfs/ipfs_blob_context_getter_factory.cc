/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_blob_context_getter_factory.h"

#include <utility>

#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"

namespace ipfs {

IpfsBlobContextGetterFactory::IpfsBlobContextGetterFactory(
    content::BrowserContext* browser_context) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  getter_callback_ = browser_context->GetBlobStorageContext();
  DCHECK(getter_callback_);
}

IpfsBlobContextGetterFactory::~IpfsBlobContextGetterFactory() {}

base::WeakPtr<storage::BlobStorageContext>
IpfsBlobContextGetterFactory::RetrieveStorageContext() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  return getter_callback_.Run();
}

}  // namespace ipfs
