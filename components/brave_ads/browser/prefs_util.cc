/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/prefs_util.h"

namespace brave_ads {
namespace prefs {

const base::Value* GetValue(PrefService* prefs, const std::string& path) {
  DCHECK(prefs);
  DCHECK(!path.empty());
  DCHECK(prefs->FindPreference(path));

  const base::Value* value = prefs->Get(path);

  if (!prefs->HasPrefPath(path)) {
    // If the preference path does not exist then the default value set with
    // Register<Type>Pref has not been serialized, so we need to serialize the
    // default value to allow upgrade paths where the default value changes
    prefs->Set(path, *value);
  }

  // If the preference path does exist then a value was serialized, so return
  // the serialized value
  return value;
}

}  // namespace prefs
}  // namespace brave_ads
