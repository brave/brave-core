// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/preserve_container_destination.h"

namespace containers {

PreserveContainerDestination::PreserveContainerDestination(
    content::WebContents* web_contents)
    : content::WebContentsUserData<PreserveContainerDestination>(
          *web_contents) {}

PreserveContainerDestination::~PreserveContainerDestination() = default;

// static
bool PreserveContainerDestination::Consume(content::WebContents* web_contents) {
  if (!web_contents || !FromWebContents(web_contents)) {
    return false;
  }
  web_contents->RemoveUserData(UserDataKey());
  return true;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PreserveContainerDestination);

}  // namespace containers
