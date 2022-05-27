/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONSTANTS_BRAVE_SWITCHES_H_
#define BRAVE_COMPONENTS_CONSTANTS_BRAVE_SWITCHES_H_

namespace switches {

// All switches in alphabetical order. The switches should be documented
// alongside the definition of their values in the .cc file.
extern const char kDisableBraveExtension[];

extern const char kDisableBraveRewardsExtension[];

extern const char kDisableBraveUpdate[];

extern const char kDisableWebTorrentExtension[];

extern const char kDisableBraveWaybackMachineExtension[];

extern const char kRewards[];

extern const char kDarkMode[];

extern const char kDisableMachineId[];

extern const char kDisableEncryptionWin[];

extern const char kComponentUpdateIntervalInSec[];

extern const char kDisableDnsOverHttps[];

extern const char kUpdateFeedURL[];

extern const char kTor[];
}  // namespace switches

#endif  // BRAVE_COMPONENTS_CONSTANTS_BRAVE_SWITCHES_H_
