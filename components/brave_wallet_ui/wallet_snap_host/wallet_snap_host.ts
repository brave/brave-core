// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Trusted host page for the hidden snap execution environment. Binds
// mojom::SnapService and hosts chrome-untrusted://snap-host iframes; the
// WebUI itself is feature-gated, so no loadTimeData check is needed here.
import { snapHostBridgeRegistry } from '../common/snap/snap_host_bridge_registry'

document.addEventListener('DOMContentLoaded', () => {
  snapHostBridgeRegistry.ensureBridge()
})
