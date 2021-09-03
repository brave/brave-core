/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SWITCHES_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SWITCHES_H_

namespace speedreader {

// Command line stylesheet override for Speedreader. Useful for testing changes.
constexpr char kSpeedreaderStylesheet[] = "speedreader-stylesheet";

// NOTE: All of the following switches are DEPRECATED. The following command
// line arguments support the speedreader adblock backend, which is no longer in
// use. TODO(keur): Delete these.
constexpr char kSpeedreaderWhitelist[] = "speedreader-whitelist";
constexpr char kSpeedreaderWhitelistPath[] = "speedreader-whitelist-path";
constexpr char kSpeedreaderBackend[] = "speedreader-backend";

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SWITCHES_H_
