import * as React from 'react'
import SelectRegion from './components/select-region'
import MainPanel from './components/main-panel'
import apiProxy from './vpn_panel_api_proxy.js'
import { ConnectionState } from './types/connection_state'

function Main () {
  const [isOn, setOn] = React.useState(false)
  const [isSelectingRegion, setSelectingRegion] = React.useState(false)
  const [hasError, setHasError] = React.useState(false)
  const [status, setStatus] = React.useState<ConnectionState>(ConnectionState.DISCONNECTED)

  const handleToggleClick = () => setOn(prevState => !prevState)
  const handleSelectRegionButtonClick = () => setSelectingRegion(true)
  const handleOnDone = () => setSelectingRegion(false)

  React.useEffect(() => {
    if (status === ConnectionState.CONNECT_FAILED) setHasError(true)
  }, [status])

  React.useEffect(() => {
    if (isOn) {
      apiProxy.getInstance().connect()
    } else {
      apiProxy.getInstance().disconnect()
    }
  }, [isOn])

  React.useEffect(() => {
    const visibilityChangedListener = () => {
      if (document.visibilityState === 'visible') {
        apiProxy.getInstance().showUI()
        if (hasError) {
          setHasError(false)
          setOn(false)
        }
      }
    }

    document.addEventListener('visibilitychange', visibilityChangedListener)

    apiProxy.getInstance().addVPNObserver({
      onConnectionCreated: () => {/**/},
      onConnectionRemoved: () => {/**/},
      onConnectionStateChanged: (state: ConnectionState) => setStatus(state)
    })

    return () => {
      document.removeEventListener('visibilitychange', visibilityChangedListener)
    }
  }, [hasError])

  if (isSelectingRegion) {
    return <SelectRegion onDone={handleOnDone} />
  }

  // TODO(nullhook): Create a seperate component for error state
  if (hasError) {
    return <h1>Can't connect to server, sorry!</h1>
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
