/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Import BraveNewsControllerMock first.
import './default/data/mockBraveNewsController'

import * as React from 'react'
import { Dispatch } from 'redux'
import { Provider as ReduxProvider } from 'react-redux'
// Components
import NewTabPage from '../containers/newTab'
import { getActionsForDispatch } from '../api/getActions'
import store from '../store'
import { useNewTabData, getGridSitesData } from './default/data/storybookState'
import { onChangeColoredBackground, onUseBraveBackground, onShowBrandedImageChanged } from './default/data/backgroundWallpaper'
import getTodayState from './default/data/todayStorybookState'
import getBraveVPNState from './default/data/braveVPNStorybookState'
import getBraveNewsDisplayAd from './default/data/getBraveNewsDisplayAd'
import { getDataUrl, getUnpaddedAsDataUrl } from '../../common/privateCDN'
import { images, updateImages } from '../data/backgrounds'

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

const StoreProvider: React.FC<React.PropsWithChildren> = ({ children }) => {
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

updateImages(images.map((image): NewTab.BraveBackground => {
  return {
    ...image,
    wallpaperImageUrl: require('../../img/newtab/backgrounds/' + image.wallpaperImageUrl)
  }
}))

export const Regular = () => {
  const doNothing = (value?: any) => value
  const state = store.getState()
  const newTabData = useNewTabData(state.newTabData)
  const gridSitesData = getGridSitesData(state.gridSitesData)
  const todayState = getTodayState()
  const braveVPNState = getBraveVPNState()

  return (
    <NewTabPage
      newTabData={newTabData}
      todayData={todayState}
      braveVPNData={braveVPNState}
      gridSitesData={gridSitesData}
      actions={getActions()}
      saveShowBackgroundImage={doNothing}
      saveShowRewards={doNothing}
      saveShowBraveTalk={doNothing}
      saveBrandedWallpaperOptIn={onShowBrandedImageChanged}
      saveSetAllStackWidgets={doNothing}
      getBraveNewsDisplayAd={getBraveNewsDisplayAd}
      setBraveBackground={onUseBraveBackground}
      chooseNewCustomBackgroundImage={doNothing}
      setCustomImageBackground={doNothing}
      removeCustomImageBackground={doNothing}
      setColorBackground={onChangeColoredBackground}
    />
  )
}
