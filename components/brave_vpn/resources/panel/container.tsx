import * as React from 'react'

import MainPanel from './components/main-panel'
import SellPanel from './components/sell-panel'
import { ViewType } from './state/component_types'
import { useSelector } from './state/hooks'

function Main () {
  const currentView = useSelector(state => state.currentView)

  if (currentView === ViewType.Main) {
    return (<MainPanel />)
  }

  return (
    <SellPanel />
  )
}

export default Main
