/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../extensions/browser/extension_prefs.cc"
namespace extensions {

namespace {
constexpr const char kPrefTorEnabled[] = "tor";
}  // namespace

bool ExtensionPrefs::IsTorEnabled(const std::string& extension_id) const {
  return ReadPrefAsBooleanAndReturn(extension_id, kPrefTorEnabled);
}

void ExtensionPrefs::SetIsTorEnabled(const std::string& extension_id,
                                           bool enabled) {
  UpdateExtensionPref(extension_id, kPrefTorEnabled,
                      std::make_unique<base::Value>(enabled));
}

}  // namespace extensions
