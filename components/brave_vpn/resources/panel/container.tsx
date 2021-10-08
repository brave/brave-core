import * as React from 'react'
import SelectRegionList from './components/select-region-list'
import MainPanel from './components/main-panel'
import ErrorPanel from './components/error-panel'
import apiProxy from './api/vpn_panel_api_proxy.js'
import { ConnectionState } from './api/panel_browser_api'
import { RegionState, Region } from './api/region_interface'

function Main () {
  const [isOn, setOn] = React.useState(false)
  const [isSelectingRegion, setSelectingRegion] = React.useState(false)
  const [hasError, setHasError] = React.useState(false)
  const [status, setStatus] = React.useState<ConnectionState>(ConnectionState.DISCONNECTED)
  const [region, setRegion] = React.useState<RegionState>({
    all: undefined,
    current: undefined,
    hasError: false
  })

  // VPN api actions should be performed on user interactions only
  const handleToggleClick = () => {
    setOn(prevState => {
      const newState = !prevState
      if (newState) apiProxy.getInstance().connect()
      else apiProxy.getInstance().disconnect()
      return newState
    })
  }

  const handleTryAgain = () => {
    setHasError(false)
    setOn(true)
    apiProxy.getInstance().connect()
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

  const handleConnectClick = async () => {
    if (!region.current) return
    setSelectingRegion(false)
    // TODO(nullhook): If same region is being selected then connect shouldn't
    // be triggered
    apiProxy.getInstance().setSelectedRegion(region.current)
    setOn(true)
    apiProxy.getInstance().connect()
  }

  const handleRegionItemClick = async (currentRegion: Region) => {
    setRegion(prevState => ({
      ...prevState,
      current: currentRegion
    }))
  }

  React.useEffect(() => {
    const onVisibilityChange = () => {
      if (document.visibilityState === 'visible') {
        resetUI(hasError)
        apiProxy.getInstance().showUI()
      }
    }

    document.addEventListener('visibilitychange', onVisibilityChange)

    apiProxy.getInstance().addVPNObserver({
      onConnectionCreated: () => {/**/},
      onConnectionRemoved: () => {/**/},
      onConnectionStateChanged: (state: ConnectionState) => {
        if (state === ConnectionState.CONNECT_FAILED) {
          setHasError(true)
          setOn(false)
        }

        setStatus(state)
      },
      onPurchasedStateChanged: () => {/**/}
    })

    return () => {
      document.removeEventListener('visibilitychange', onVisibilityChange)
    }
  }, [hasError])

  React.useEffect(() => {
    // All initial data must be loaded and set in this effect
    const getInitialState = async () => {
      const res = await apiProxy.getInstance().getConnectionState()
      setOn(res.state === ConnectionState.CONNECTED
        || res.state === ConnectionState.CONNECTING)
      // Treat connection failure as disconnect on initial startup
      if (res.state !== ConnectionState.CONNECT_FAILED) setStatus(res.state)
    }

    const getInitialRegion = async () => {
      try {
        const res = await apiProxy.getInstance().getSelectedRegion()
        setRegion(prevState => ({ ...prevState, current: res.currentRegion }))
      } catch (err) {
        console.error(err)
      }
    }

    const getAllRegions = async () => {
      try {
        const res = await apiProxy.getInstance().getAllRegions()
        setRegion(prevState => ({ ...prevState, all: res.regions }))
      } catch (err) {
        console.error(err)
        // TODO(nullhook): Create an error component for region list
        setRegion(prevState => ({ ...prevState, hasError: true }))
      }
    }

    getInitialState().catch(e => console.error('getConnectionState failed', e))
    getInitialRegion().catch(e => console.error('getInitialRegion failed', e))
    getAllRegions().catch(e => console.error('getAllRegions failed', e))
  }, [])

  if (!region.current) {
    // TODO(nullhook): Replace this with a loading state and move to redux
    return null
  }

  if (isSelectingRegion && region.all) {
    return (
      <SelectRegionList
        regions={region.all}
        selectedRegion={region.current}
        onConnectClick={handleConnectClick}
        onDone={onSelectingRegionDone}
        onRegionClick={handleRegionItemClick}
      />)
  }

  if (hasError) {
    return (
      <ErrorPanel
        onTryAgainClick={handleTryAgain}
        onChooseServerClick={handleSelectRegionButtonClick}
        region={region}
      />
    )
  }

  // TODO(nullhook): Show the main ui after the initial data loads and
  // if the purchased state is PURCHSED
  return (
    <MainPanel
      isOn={isOn}
      status={status}
      region={region.current}
      onToggleClick={handleToggleClick}
      onSelectRegionButtonClick={handleSelectRegionButtonClick}
    />
  )
}

export default Main
