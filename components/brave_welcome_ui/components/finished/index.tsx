// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  P3APhase,
  WelcomeBrowserProxyImpl
} from '../../api/welcome_browser_proxy'

// This component is used for Brave Origin to skip the HelpWDP and HelpImprove
// steps and finish the welcome flow immediately.
function Finished() {
  React.useEffect(() => {
    WelcomeBrowserProxyImpl.getInstance().recordP3A(P3APhase.Finished)
    WelcomeBrowserProxyImpl.getInstance().getWelcomeCompleteURL()
      .then((url) => {
        window.open(url || 'chrome://newtab', '_self', 'noopener')
      })
  }, [])

  return null
}

export default Finished
