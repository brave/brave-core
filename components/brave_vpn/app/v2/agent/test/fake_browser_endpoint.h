/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_ENDPOINT_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_ENDPOINT_H_

#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"

namespace brave_vpn::v2 {

// Stands in for the surface a browser exposes to the agent. Owns the browser's
// end of that pipe; methods get fake implementations here as they are added to
// the interface.
class FakeBrowserEndpoint : public mojom::BrowserEndpoint {
 public:
  FakeBrowserEndpoint();
  ~FakeBrowserEndpoint() override;

  FakeBrowserEndpoint(const FakeBrowserEndpoint&) = delete;
  FakeBrowserEndpoint& operator=(const FakeBrowserEndpoint&) = delete;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_ENDPOINT_H_
