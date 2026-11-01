// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import getPageHandlerInstance, { ConnectionStatus } from './api/brave_page_handler'
import { loadTimeData } from '../../../common/loadTimeData'

// Initial values come from load time data so the first paint is complete;
// mojo is only used for updates.

export function useTorObserver () {
  const [hasInit, setHasInit] = React.useState<boolean>(false)
  const [isConnected, setIsConnected] =
    React.useState(() => loadTimeData.getBoolean('isTorConnected'))
  const [progress, setProgress] = React.useState<string | undefined>(undefined)
  const [message, setMessage] = React.useState<string | undefined>(undefined)
  const [connectionStatus, setConnectionStatus] = React.useState<ConnectionStatus>(ConnectionStatus.kUnknown)

  React.useEffect(() => {
    getPageHandlerInstance().callbackRouter.onTorCircuitEstablished.addListener(setHasInit)
    getPageHandlerInstance().callbackRouter.onTorInitializing.addListener((progress: string, message: string) => {
      setProgress(progress)
      setMessage(message)
    })
    getPageHandlerInstance().callbackRouter.onTorCircuitStatus.addListener(setConnectionStatus)
  }, [])

  React.useEffect(() => {
    getPageHandlerInstance().pageHandler.getIsTorConnected()
      .then(res => setIsConnected(res.isConnected))
  }, [hasInit])

  return {
    isConnected,
    progress,
    message,
    connectionStatus
  }
}

export function useInfoCardsDismissed () {
  const [isDismissed, setIsDismissed] =
    React.useState(() => loadTimeData.getBoolean('infoCardsDismissed'))

  const setDismissed = React.useCallback((dismissed: boolean) => {
    setIsDismissed(dismissed)
    getPageHandlerInstance().pageHandler.setDisclaimerDismissed(dismissed)
  }, [])

  return { isDismissed, setDismissed }
}
