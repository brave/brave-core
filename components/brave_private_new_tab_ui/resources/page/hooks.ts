// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import getPageHandlerInstance, { ConnectionStatus } from './api/brave_page_handler'

export function useTorObserver () {
  const [hasInit, setHasInit] = React.useState<boolean>(false)
  const [isConnected, setIsConnected] = React.useState(false)
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

export function useHasDisclaimerDismissed () {
  // We use a tri-state here to help render disclaimer UI only if there are non-null values,
  // which happens after promise is resolved
  const [hasDisclaimerDismissed, setHasDisclaimerDismissed] = React.useState<boolean | null>(null)

  React.useEffect(() => {
    getPageHandlerInstance().pageHandler.getDisclaimerDismissed()
      .then(res => setHasDisclaimerDismissed(res.dismissed))
  }, [])

  return { hasDisclaimerDismissed }
}
