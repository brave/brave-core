/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_VIRTUAL_PREF_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_VIRTUAL_PREF_PROVIDER_H_

#include <memory>
#include <string>
#include <string_view>

#include "base/memory/raw_ptr.h"
#include "base/values.h"

class PrefService;

namespace brave_ads {

class VirtualPrefProvider final {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual std::string_view GetChannel() const = 0;

    virtual std::string GetDefaultSearchEngineName() const = 0;
  };

  VirtualPrefProvider(PrefService* prefs,
                      PrefService* local_state,
                      std::unique_ptr<Delegate> delegate);

  VirtualPrefProvider(const VirtualPrefProvider&) = delete;
  VirtualPrefProvider& operator=(const VirtualPrefProvider&) = delete;

  ~VirtualPrefProvider();

  base::Value::Dict GetPrefs() const;

 private:
  const raw_ptr<PrefService> prefs_;        // Not owned.
  const raw_ptr<PrefService> local_state_;  // Not owned.

  const std::unique_ptr<Delegate> delegate_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_VIRTUAL_PREF_PROVIDER_H_
