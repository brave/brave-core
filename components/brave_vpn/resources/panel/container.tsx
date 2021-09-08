import * as React from 'react'
import SelectRegion from './components/select-region'
import MainPanel from './components/main-panel'
import ErrorPanel from './components/error-panel'
import apiProxy from './vpn_panel_api_proxy.js'
import { ConnectionState } from './types/connection_state'

function Main () {
  const [isOn, setOn] = React.useState(false)
  const [isSelectingRegion, setSelectingRegion] = React.useState(false)
  const [hasError, setHasError] = React.useState(false)
  const [status, setStatus] = React.useState<ConnectionState>(ConnectionState.DISCONNECTED)

  const handleToggleClick = () => {
    // VPN actions should be performed on user interactions
    setOn(prevState => {
      const newState = !prevState
      if (newState) apiProxy.getInstance().connect()
      else apiProxy.getInstance().disconnect()
      return newState
    })
  }

  const handleSelectRegionButtonClick = () => setSelectingRegion(true)

  const resetUI = (state: boolean) => {
    if (state) {
      setOn(false)
      setHasError(false)
    }
  }

  const onSelectingRegionDone = () => {
    resetUI(hasError)
    setSelectingRegion(false)
  }

  const handleTryAgain = () => {
    setHasError(false)
    setOn(true)
    apiProxy.getInstance().connect()
  }

  React.useEffect(() => {
    const visibilityChangedListener = () => {
      if (document.visibilityState === 'visible') {
        resetUI(hasError)
        apiProxy.getInstance().showUI()
      }
    }

    document.addEventListener('visibilitychange', visibilityChangedListener)

    apiProxy.getInstance().addVPNObserver({
      onConnectionCreated: () => {/**/},
      onConnectionRemoved: () => {/**/},
      onConnectionStateChanged: (state: ConnectionState) => {
        if (state === ConnectionState.CONNECT_FAILED) {
          setHasError(true)
          setOn(false)
        }

        setStatus(state)
      }
    })

    return () => {
      document.removeEventListener('visibilitychange', visibilityChangedListener)
    }
  }, [hasError])

  React.useEffect(() => {
    const getInitialState = async () => {
      const res = await apiProxy.getInstance().getConnectionState()
      setOn(res.state === ConnectionState.CONNECTED
        || res.state === ConnectionState.CONNECTING)
      // Treat connection failure as disconnect on initial startup
      if (res.state !== ConnectionState.CONNECT_FAILED) setStatus(res.state)
    }

    getInitialState().catch(e => console.error('getConnectionState failed', e))
  }, [])

  if (isSelectingRegion) {
    return <SelectRegion onDone={onSelectingRegionDone} />
  }

  if (hasError) {
    return (
      <ErrorPanel
        onTryAgainClick={handleTryAgain}
        onChooseServerClick={handleSelectRegionButtonClick}
        region='Tokyo'
      />
    )
  }

  return (
    <MainPanel
      isOn={isOn}
      status={status}
      region='Tokyo' // TODO(nullhook): Transfer to a stateful value
      onToggleClick={handleToggleClick}
      onSelectRegionButtonClick={handleSelectRegionButtonClick}
    />
  )
}

export default Main
