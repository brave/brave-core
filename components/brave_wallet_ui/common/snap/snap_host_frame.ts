// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { loadTimeData } from '../../../common/loadTimeData'

const kSnapHostFrameId = 'snaps-container-frame'

// Adds the hidden chrome://snaps-container/ frame that owns the SnapHostBridge
// and parents the untrusted snap frames. Idempotent.
export const ensureSnapHostFrame = () => {
  if (document.getElementById(kSnapHostFrameId)) {
    return
  }
  const element = document.createElement('iframe')
  element.id = kSnapHostFrameId
  element.src = loadTimeData.getString('braveSnapsContainerUrl')
  element.style.display = 'none'
  // Deliberately unsandboxed: the frame needs its real chrome://snaps-container
  // origin for Mojo bindings, for the untrusted frame's frame-ancestors check,
  // and for its postMessage origin checks. The untrusted frame inside it is the
  // isolation boundary and sandboxes itself.
  document.body.appendChild(element)
}
