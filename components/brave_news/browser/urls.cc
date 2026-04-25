// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/urls.h"

#include <string>

#include "brave/brave_domains/service_domains.h"

namespace brave_news {

std::string GetHostname() {
  return brave_domains::GetServicesDomain("brave-today-cdn");
}

}  // namespace brave_news
