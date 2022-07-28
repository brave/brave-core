import * as React from 'react'
import getPageHandlerInstance from './api/brave_page_handler'

export function useTorObserver () {
  const [hasInit, setHasInit] = React.useState<boolean>(false)
  const [isConnected, setIsConnected] = React.useState(false)
  const [progress, setProgress] = React.useState<string | undefined>(undefined)
  const [message, setMessage] = React.useState<string | undefined>(undefined)
  const [connectionFailed, setFailed] = React.useState<boolean>(false)

  const isLoading = !hasInit && !isConnected

  React.useEffect(() => {
    getPageHandlerInstance().callbackRouter.onTorCircuitEstablished.addListener(setHasInit)
    getPageHandlerInstance().callbackRouter.onTorInitializing.addListener((progress: string, message: string) => {
      setProgress(progress)
      setMessage(message)
    })
    getPageHandlerInstance().callbackRouter.onTorCircuitGetStuck.addListener(setFailed)
  }, [])

  React.useEffect(() => {
    getPageHandlerInstance().pageHandler.getIsTorConnected()
      .then(res => setIsConnected(res.isConnected))
  }, [hasInit])

  return {
    isConnected,
    isLoading,
    progress,
    message,
    connectionFailed,
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
