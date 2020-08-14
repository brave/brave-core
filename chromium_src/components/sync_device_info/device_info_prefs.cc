// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "../../../../components/sync_device_info/device_info_prefs.cc"

namespace syncer {

PrefService* DeviceInfoPrefs::GetPrefService() {
  return pref_service_;
}

}  // namespace syncer
