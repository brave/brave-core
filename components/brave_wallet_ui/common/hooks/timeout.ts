// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

export default function useTimeout (
  notifyUserInteraction: () => void,
  isWalletLocked: boolean
) {
  const [lastCheckedTime, setLastCheckedTime] = React.useState<number>()
  const [currentMousePosition, setCurrentMousePosition] = React.useState({ x: 0, y: 0 })
  const [lastMousePosition, setLastMousePosition] = React.useState({ x: 0, y: 0 })
  React.useMemo(() => {
    const { x, y } = currentMousePosition
    // Checks to see if the mouse moved everytime lastCheckTime is updated
    if (lastMousePosition.x !== x || lastMousePosition.y !== y) {
      setLastMousePosition({ x, y })
      // If mouse has moved will notify keyring here.
      notifyUserInteraction()
    }
  }, [lastCheckedTime])

  React.useEffect(() => {
    // Starts tracking mouse move events when wallet is unlocked
    if (!isWalletLocked) {
      const mousePosition = (event: MouseEvent) => setCurrentMousePosition({ x: event.clientX, y: event.clientY })
      window.addEventListener('mousemove', mousePosition)
      return () => window.removeEventListener('mousemove', mousePosition)
    }
    return
  }, [isWalletLocked])

  React.useEffect(() => {
    // If wallet is unlocked, will start interval
    // and setLastCheckedTime every 50 seconds to trigger
    // a check to see if the mouse moved.
    if (!isWalletLocked) {
      const interval = setInterval(() => {
        const date = Date.now()
        setLastCheckedTime(date)
      }, 50000)
      return () => clearInterval(interval)
    }
    return
  }, [isWalletLocked])
}
