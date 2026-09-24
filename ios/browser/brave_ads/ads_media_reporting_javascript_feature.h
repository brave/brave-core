// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_MEDIA_REPORTING_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_MEDIA_REPORTING_JAVASCRIPT_FEATURE_H_

#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace brave_ads {

class AdsMediaReportingJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static AdsMediaReportingJavaScriptFeature* GetInstance();

  // JavaScriptFeature:
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  void ScriptMessageReceived(web::WebState* web_state,
                             const web::ScriptMessage& message) override;

 private:
  friend class base::NoDestructor<AdsMediaReportingJavaScriptFeature>;

  AdsMediaReportingJavaScriptFeature();
  ~AdsMediaReportingJavaScriptFeature() override;
};

}  // namespace brave_ads

#endif  // BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_MEDIA_REPORTING_JAVASCRIPT_FEATURE_H_
