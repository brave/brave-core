// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>

#include "brave/components/brave_today/browser/network.h"

namespace brave_news {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("brave_news_controller", R"(
      semantics {
        sender: "Brave News Controller"
        description:
          "This controller is used to fetch brave news feeds and publisher lists."
        trigger:
          "Triggered by uses of the Brave News feature."
        data:
          "Article JSON"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature on the New Tab Page customization."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

}  // namespace brave_news
