// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/network.h"

#include <string>

#include "base/time/time.h"

namespace brave_news {

base::TimeDelta GetDefaultRequestTimeout() {
  return base::Seconds(30.0);
}
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
