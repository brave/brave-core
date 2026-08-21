// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_PRESERVE_CONTAINER_DESTINATION_H_
#define BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_PRESERVE_CONTAINER_DESTINATION_H_

#include "base/component_export.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}  // namespace content

namespace containers {

// Marker set when NavigateParams::preserve_container_destination is true
// (explicit open in / without / temporary container UI). Traffic Control
// skips re-routing while present. Consumed on first check so later navigations
// in the same tab are subject to rules again.
class COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER) PreserveContainerDestination
    : public content::WebContentsUserData<PreserveContainerDestination> {
 public:
  ~PreserveContainerDestination() override;

  // Returns true if |web_contents| has the marker, and removes it (one-shot).
  static bool Consume(content::WebContents* web_contents);

 private:
  friend content::WebContentsUserData<PreserveContainerDestination>;

  explicit PreserveContainerDestination(content::WebContents* web_contents);

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_PRESERVE_CONTAINER_DESTINATION_H_
