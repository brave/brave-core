// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_PRIVATE_CDN_PRIVATE_CDN_REQUEST_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_PRIVATE_CDN_PRIVATE_CDN_REQUEST_HELPER_H_

#include <list>
#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_private_cdn {

// PrivateCDNRequestHelper ensures that the request is made anonymous,
// and reduces boilerplate.
class PrivateCDNRequestHelper {
 public:
  PrivateCDNRequestHelper(
      net::NetworkTrafficAnnotationTag annotation_tag,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~PrivateCDNRequestHelper();

  using DownloadToStringCallback =
      base::OnceCallback<void(const int, const std::string&)>;

  void DownloadToString(const GURL& url,
                        DownloadToStringCallback callback,
                        bool auto_retry_on_network_change = true,
                        size_t max_body_size = 5 * 1024 * 1024 /* 5mb */);

 private:
  PrivateCDNRequestHelper(const PrivateCDNRequestHelper&) = delete;
  PrivateCDNRequestHelper& operator=(const PrivateCDNRequestHelper&) = delete;
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  void OnResponse(SimpleURLLoaderList::iterator iter,
                  DownloadToStringCallback callback,
                  const std::unique_ptr<std::string> response_body);

  net::NetworkTrafficAnnotationTag annotation_tag_;
  SimpleURLLoaderList url_loaders_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WeakPtrFactory<PrivateCDNRequestHelper> weak_ptr_factory_{this};
};

}  // namespace brave_private_cdn

#endif  // BRAVE_COMPONENTS_BRAVE_PRIVATE_CDN_PRIVATE_CDN_REQUEST_HELPER_H_
