// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_WEBUI_WEBUI_RESOURCES_H_
#define BRAVE_COMPONENTS_WEBUI_WEBUI_RESOURCES_H_

#include <string_view>

#include "base/containers/span.h"

namespace webui {

struct LocalizedString;
struct ResourcePath;

}  // namespace webui

namespace brave {

base::span<const webui::ResourcePath> GetWebUIResources(
    std::string_view webui_name);

base::span<const webui::LocalizedString> GetWebUILocalizedStrings(
    std::string_view webui_name);

}  // namespace brave

#endif  // BRAVE_COMPONENTS_WEBUI_WEBUI_RESOURCES_H_
