/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sessions/content/content_serialized_navigation_driver.h"

#include <string>

#include "content/public/common/url_constants.h"

namespace sessions {

class SerializedNavigationEntry;

// Defined in //brave/components/sessions/content:chromium_impl.
std::string BraveSanitizePageStateForPickle(
    const SerializedNavigationEntry& navigation,
    std::string page_state);
void BraveSanitizeNavigation(SerializedNavigationEntry& navigation);

}  // namespace sessions

#include <components/sessions/content/content_serialized_navigation_driver.cc>
