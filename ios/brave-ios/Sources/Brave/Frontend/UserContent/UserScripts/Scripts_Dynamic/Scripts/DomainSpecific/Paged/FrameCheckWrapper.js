// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

window.__firefox__.execute(function ($) {
  (_ => {
    // Because we can't inject scripts to specific frames (except main frame),
    // we need to check if this script belongs to this specific frame
    // so that it doesn't execute on the wrong frame.
    const requiredHref = "$<required_href>"
    if (window.origin !== requiredHref) {
      return
    }

    $<scriplet>
  })()
})
