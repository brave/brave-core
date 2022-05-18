/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SHARING_HUB_BRAVE_SHARING_HUB_MODEL_H_
#define BRAVE_BROWSER_SHARING_HUB_BRAVE_SHARING_HUB_MODEL_H_

#include <vector>
#include "chrome/browser/sharing_hub/sharing_hub_model.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace sharing_hub {
class BraveSharingHubModel : public sharing_hub::SharingHubModel {
 public:
  explicit BraveSharingHubModel(content::BrowserContext* context);
  BraveSharingHubModel(const BraveSharingHubModel&) = delete;
  BraveSharingHubModel& operator=(const BraveSharingHubModel&) = delete;
  ~BraveSharingHubModel() override;

  void GetFirstPartyActionList(content::WebContents* web_contents,
                               std::vector<SharingHubAction>* list) override;

 private:
  SharingHubAction brave_talk_share_tab_action_;
};
}  // namespace sharing_hub

#endif  // BRAVE_BROWSER_SHARING_HUB_BRAVE_SHARING_HUB_MODEL_H_
