// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/sessions/content/content_serialized_navigation_builder.h"

namespace content {
class NavigationEntry;
}  // namespace content

namespace sessions {

class SerializedNavigationEntry;

// Defined in //brave/components/sessions/content:chromium_impl.
void BraveSerializeNavigationEntry(content::NavigationEntry* entry,
                                   SerializedNavigationEntry* navigation);
void BraveRestoreNavigationEntry(const SerializedNavigationEntry* navigation,
                                 content::NavigationEntry* entry);

}  // namespace sessions

#include <components/sessions/content/content_serialized_navigation_builder.cc>
