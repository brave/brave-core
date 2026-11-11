// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// The browser counts a conversation as viewed only while a client reports it's
// on screen, so report whenever that changes: the page's visibility, or whether
// the UI is showing the conversation. Nothing is reported on cleanup: the next
// run reports straight away, and the browser clears a closed binding itself.
export default function useReportClientVisibility(
  setClientVisible: (visible: boolean) => void,
  isConversationShown: boolean,
) {
  React.useEffect(() => {
    const report = () =>
      setClientVisible(
        isConversationShown && document.visibilityState === 'visible',
      )
    report()
    document.addEventListener('visibilitychange', report)
    return () => document.removeEventListener('visibilitychange', report)
  }, [setClientVisible, isConversationShown])
}
