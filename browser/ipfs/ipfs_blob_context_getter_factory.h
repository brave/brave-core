/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_BLOB_CONTEXT_GETTER_FACTORY_H_
#define BRAVE_BROWSER_IPFS_IPFS_BLOB_CONTEXT_GETTER_FACTORY_H_

#include "brave/components/ipfs/blob_context_getter_factory.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace ipfs {

class IpfsBlobContextGetterFactory : public ipfs::BlobContextGetterFactory {
 public:
  explicit IpfsBlobContextGetterFactory(content::BrowserContext* context);
  IpfsBlobContextGetterFactory(const IpfsBlobContextGetterFactory&) = delete;
  IpfsBlobContextGetterFactory& operator=(const IpfsBlobContextGetterFactory&) =
      delete;
  ~IpfsBlobContextGetterFactory() override;

 private:
  base::WeakPtr<storage::BlobStorageContext> RetrieveStorageContext() override;

  ipfs::BlobContextGetter getter_callback_;
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_BLOB_CONTEXT_GETTER_FACTORY_H_
