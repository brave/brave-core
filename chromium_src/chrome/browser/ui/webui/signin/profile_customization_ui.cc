// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

namespace content {
class WebUIDataSource;
}

namespace brave {
// Supplies the custom profile image feature value and strings to the upstream
// page. Implemented in //brave/browser/ui/webui/signin.
void AddProfileCustomizationData(content::WebUIDataSource* source);
}  // namespace brave

#include <chrome/browser/ui/webui/signin/profile_customization_ui.cc>
