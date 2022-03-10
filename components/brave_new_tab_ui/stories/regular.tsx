/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Dispatch } from 'redux'
import { Provider as ReduxProvider } from 'react-redux'
// Components
import NewTabPage from '../containers/newTab'
import { getActionsForDispatch } from '../api/getActions'
import store from '../store'
import { getNewTabData, getGridSitesData } from './default/data/storybookState'
import getTodayState from './default/data/todayStorybookState'
import getBraveNewsDisplayAd from './default/data/getBraveNewsDisplayAd'
import { getDataUrl, getUnpaddedAsDataUrl } from '../../common/privateCDN'
import getFTXStorybookState from '../widgets/ftx/ftx_storybook_state'

const doNothingDispatch: Dispatch = (action: any) => action

function getActions () {
  return getActionsForDispatch(doNothingDispatch)
}

// @ts-expect-error
window.braveStorybookUnpadUrl = async function UnpadUrl (paddedUrl: string, mimeType = 'image/jpg'): Promise<string> {
  const response = await fetch(paddedUrl)
  const blob = await response.blob()
  const buffer = await blob.arrayBuffer()
  if (paddedUrl.endsWith('.pad')) {
    return await getUnpaddedAsDataUrl(buffer, mimeType)
  }
  // Image is already unpadded
  return await getDataUrl(buffer)
}

const StoreProvider: React.FunctionComponent = ({ children }) => {
  return (
    <ReduxProvider store={store}>
     {children}
    </ReduxProvider>
  )
}

export default {
  title: 'New Tab',
  decorators: [
    (Story: any) => <StoreProvider><Story /></StoreProvider>
  ]
}

export const Regular = () => {
  const doNothing = (value: boolean) => value
  const state = store.getState()
  const newTabData = getNewTabData(state.newTabData)
  const gridSitesData = getGridSitesData(state.gridSitesData)
  const todayState = getTodayState()
  return (
    <NewTabPage
      ftx={getFTXStorybookState()}
      newTabData={newTabData}
      todayData={todayState}
      gridSitesData={gridSitesData}
      actions={getActions()}
      saveShowBackgroundImage={doNothing}
      saveShowStats={doNothing}
      saveShowToday={doNothing}
      saveShowRewards={doNothing}
      saveShowBinance={doNothing}
      saveShowBraveTalk={doNothing}
      saveShowGemini={doNothing}
      saveShowCryptoDotCom={doNothing}
      saveShowFTX={doNothing}
      saveBrandedWallpaperOptIn={doNothing}
      saveSetAllStackWidgets={doNothing}
      getBraveNewsDisplayAd={getBraveNewsDisplayAd}
    />
  )
}
