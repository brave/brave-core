// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/chrome/browser/tabs/model/tab_helper_util.h"

namespace ios_web_view {

void AttachTabHelpers(web::WebState* web_state) {
  AttachTabHelpers(web_state, TabHelperFilter::kEmpty);
}

}  // namespace ios_web_view
