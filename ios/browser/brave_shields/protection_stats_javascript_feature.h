// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_PROTECTION_STATS_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_PROTECTION_STATS_JAVASCRIPT_FEATURE_H_

#include <optional>
#include <string>

#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {
class WebState;
}  // namespace web

namespace brave_shields {

// Injects the protection stats script which reports the resources loaded by a
// page so Shields can track and block trackers, ads, and images.
class ProtectionStatsJavaScriptFeature : public web::JavaScriptFeature {
 public:
  static ProtectionStatsJavaScriptFeature* GetInstance();

  // JavaScriptFeature:
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  void ScriptMessageReceived(web::WebState* web_state,
                             const web::ScriptMessage& message) override;

 private:
  friend class base::NoDestructor<ProtectionStatsJavaScriptFeature>;

  ProtectionStatsJavaScriptFeature();
  ~ProtectionStatsJavaScriptFeature() override;

  ProtectionStatsJavaScriptFeature(const ProtectionStatsJavaScriptFeature&) =
      delete;
  ProtectionStatsJavaScriptFeature& operator=(
      const ProtectionStatsJavaScriptFeature&) = delete;
};

}  // namespace brave_shields

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_PROTECTION_STATS_JAVASCRIPT_FEATURE_H_
