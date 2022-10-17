/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sharing_hub/sharing_hub_model.h"
#include "chrome/browser/sharing_hub/sharing_hub_service.h"
#include "chrome/browser/sharing_hub/sharing_hub_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

using BraveSharingHubTest = InProcessBrowserTest;

namespace sharing_hub {

IN_PROC_BROWSER_TEST_F(BraveSharingHubTest,
                       SharingHubThirdPartyActionsEmptyTest) {
  auto* profile = browser()->profile();

  EXPECT_TRUE(
      profile->GetPrefs()->GetBoolean(prefs::kDesktopSharingHubEnabled));

  auto* sharing_hub_service = SharingHubServiceFactory::GetForProfile(profile);
  auto* model = sharing_hub_service->GetSharingHubModel();
  std::vector<SharingHubAction> list;
  model->GetThirdPartyActionList(&list);
  EXPECT_TRUE(list.empty());
}

}  // namespace sharing_hub
