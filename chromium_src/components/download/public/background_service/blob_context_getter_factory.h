/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_BLOB_CONTEXT_GETTER_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_BLOB_CONTEXT_GETTER_FACTORY_H_

#include "src/components/download/public/background_service/blob_context_getter_factory.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace download {

using URLLoaderFactoryGetterCallback =
    base::OnceCallback<void(scoped_refptr<network::SharedURLLoaderFactory>)>;

class COMPONENT_EXPORT(COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE)
    URLLoaderFactoryGetter {
 public:
  virtual void RetrieveURLLoader(URLLoaderFactoryGetterCallback callback) = 0;

  URLLoaderFactoryGetter(const URLLoaderFactoryGetter&) = delete;
  URLLoaderFactoryGetter& operator=(const URLLoaderFactoryGetter&) = delete;

  virtual ~URLLoaderFactoryGetter() = default;

 protected:
  URLLoaderFactoryGetter() = default;
};

using URLLoaderFactoryGetterPtr = std::unique_ptr<URLLoaderFactoryGetter>;

}  // namespace download

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_BLOB_CONTEXT_GETTER_FACTORY_H_
