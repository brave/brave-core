// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_IMPL_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_IMPL_H_

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace brave_rewards {

class SkusSdkImpl {
 public:
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_IMPL_H_
