import * as React from 'react'

import MainPanel from './components/main-panel'
import SellPanel from './components/sell-panel'
import LoadingPanel from './components/loading-panel'
import ErrorSubscriptionExpired from './components/error-subscription-failed-panel'
import { ViewType } from './state/component_types'
import { useSelector } from './state/hooks'

function Main () {
  const currentView = useSelector(state => state.currentView)

  if (currentView === ViewType.Loading) {
    return (
      <LoadingPanel />
    )
  }

  if (currentView === ViewType.Main) {
    return (
      <MainPanel />
    )
  }

  if (currentView === ViewType.Expired) {
    return (
      <ErrorSubscriptionExpired />
    )
  }

  return (
    <SellPanel />
  )
}

export default Main
