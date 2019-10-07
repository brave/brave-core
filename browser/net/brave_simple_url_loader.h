/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_SIMPLE_URL_LOADER_H_
#define BRAVE_BROWSER_NET_BRAVE_SIMPLE_URL_LOADER_H_

#include "services/network/public/cpp/simple_url_loader.h"

namespace net {
struct NetworkTrafficAnnotationTag;
}  // namespace net

namespace network {

struct ResourceRequest;

class BraveSimpleURLLoader : public SimpleURLLoader {
 public:
  static std::unique_ptr<BraveSimpleURLLoader> Create(
      std::unique_ptr<ResourceRequest> resource_request,
      const net::NetworkTrafficAnnotationTag& annotation_tag);

 protected:
  BraveSimpleURLLoader();

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveSimpleURLLoader);	
};

}  // namespace network

#endif  // BRAVE_BROWSER_NET_BRAVE_SIMPLE_URL_LOADER_H_
