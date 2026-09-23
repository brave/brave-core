/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_HELPER_HELPER_APP_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_HELPER_HELPER_APP_H_

namespace brave_vpn {
namespace v2 {

// The HelperApp class encapsulates the main application logic for the
// privileged Brave VPN helper process. It provides a stub method for running
// the main event loop.

class HelperApp final {
 public:
  HelperApp();
  ~HelperApp();

  HelperApp(const HelperApp&) = delete;
  HelperApp& operator=(const HelperApp&) = delete;

  // Runs the main event loop of the application. The return value is the exit
  // code of the application.
  int Run();
};

}  // namespace v2
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_HELPER_HELPER_APP_H_
