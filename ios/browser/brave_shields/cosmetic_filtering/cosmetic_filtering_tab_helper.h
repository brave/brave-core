// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_COSMETIC_FILTERING_COSMETIC_FILTERING_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_COSMETIC_FILTERING_COSMETIC_FILTERING_TAB_HELPER_H_

#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_args.h"
#include "ios/web/public/web_state_user_data.h"

class GURL;
@protocol CosmeticFilteringTabHelperBridge;

namespace web {
class WebState;
}

class CosmeticFilteringTabHelper
    : public web::WebStateUserData<CosmeticFilteringTabHelper> {
 public:
  ~CosmeticFilteringTabHelper() override;

  void SetBridge(id<CosmeticFilteringTabHelperBridge> bridge);

  void CosmeticFilteringArgsFor(
      const GURL& url,
      base::OnceCallback<void(CosmeticFilteringArgs*)> callback);

  // `standard_selectors` and `aggressive_selectors` are null when no result
  // could be computed (e.g. no bridge is attached).
  void SelectorsToHideFor(
      const GURL& url,
      const std::vector<std::string>& ids,
      const std::vector<std::string>& classes,
      base::OnceCallback<
          void(const std::vector<std::string>* standard_selectors,
               const std::vector<std::string>* aggressive_selectors)> callback);

 private:
  friend class web::WebStateUserData<CosmeticFilteringTabHelper>;

  explicit CosmeticFilteringTabHelper(web::WebState* web_state);

  __weak id<CosmeticFilteringTabHelperBridge> bridge_;
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_COSMETIC_FILTERING_COSMETIC_FILTERING_TAB_HELPER_H_
