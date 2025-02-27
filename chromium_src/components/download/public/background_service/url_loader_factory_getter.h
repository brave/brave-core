/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_URL_LOADER_FACTORY_GETTER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_URL_LOADER_FACTORY_GETTER_H_

#include "base/component_export.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace download {

using URLLoaderFactoryGetterCallback =
    base::OnceCallback<void(scoped_refptr<network::SharedURLLoaderFactory>)>;

class COMPONENT_EXPORT(COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE)
    URLLoaderFactoryGetter {
 public:
  virtual void RetrieveURLLoaderFactory(
      URLLoaderFactoryGetterCallback callback) = 0;

  URLLoaderFactoryGetter(const URLLoaderFactoryGetter&) = delete;
  URLLoaderFactoryGetter& operator=(const URLLoaderFactoryGetter&) = delete;

  virtual ~URLLoaderFactoryGetter() = default;

 protected:
  URLLoaderFactoryGetter() = default;
};

using URLLoaderFactoryGetterPtr = std::unique_ptr<URLLoaderFactoryGetter>;

}  // namespace download

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_URL_LOADER_FACTORY_GETTER_H_
