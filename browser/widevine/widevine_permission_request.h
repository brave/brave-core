/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_
#define BRAVE_BROWSER_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_

#include "base/gtest_prod_util.h"
#include "components/permissions/permission_request.h"

#include "base/memory/raw_ptr.h"
#include "url/gurl.h"

class PrefService;

class WidevinePermissionRequest : public permissions::PermissionRequest {
 public:
  WidevinePermissionRequest(PrefService* prefs,
                            const GURL& requesting_origin,
                            bool for_restart);

  WidevinePermissionRequest(const WidevinePermissionRequest&) = delete;
  WidevinePermissionRequest& operator=(const WidevinePermissionRequest&) =
      delete;

  ~WidevinePermissionRequest() override;

  std::u16string GetExplanatoryMessageText() const;

 private:
  FRIEND_TEST_ALL_PREFIXES(WidevinePermissionRequestBrowserTest,
                           TriggerTwoPermissionTest);
  static bool is_test_;

  // PermissionRequest overrides:
#if BUILDFLAG(IS_ANDROID)
  PermissionRequest::AnnotatedMessageText GetDialogAnnotatedMessageText(
      const GURL& embedding_origin) const override;
#else
  std::u16string GetMessageTextFragment() const override;
#endif
  void PermissionDecided(
      PermissionDecision decision,
      bool is_one_time,
      bool is_final_decision,
      const permissions::PermissionRequestData& request_data);

  raw_ptr<PrefService> prefs_ = nullptr;

  // Only can be true on linux.
  // On linux, browser will use another permission request buble after finishing
  // installation to ask user about restarting because installed widevine can
  // only be used after re-launch.
  bool for_restart_ = false;
};

#endif  // BRAVE_BROWSER_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_
