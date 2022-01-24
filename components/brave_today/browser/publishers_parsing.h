// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_PUBLISHERS_PARSING_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_PUBLISHERS_PARSING_H_

#include <string>

#include "brave/components/brave_today/browser/publishers_controller.h"

namespace brave_news {

bool ParsePublisherList(const std::string& json, Publishers* publishers);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_PUBLISHERS_PARSING_H_
