/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Dispatch } from 'redux'
import { Provider as ReduxProvider } from 'react-redux'
import { storiesOf } from '@storybook/react'
import { withKnobs, select } from '@storybook/addon-knobs/react'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'

// Components
import NewTabPage from '../containers/newTab'
import { getActionsForDispatch } from '../api/getActions'
import store from '../store'
import { getNewTabData, getGridSitesData } from './default/data/storybookState'
import getTodayState from './default/data/todayStorybookState'

const doNothingDispatch: Dispatch = (action: any) => action

function getActions () {
  return getActionsForDispatch(doNothingDispatch)
}

// Uncomment to use actual images proxied from a CORS-breaker proxy
// TODO(petemill): privateCDN should be in /common/
// import { getUnpaddedAsDataUrl } from '../../brave_extension/extension/brave_extension/background/today/privateCDN'

// TODO(petemill): Have the private CDN contain a CORS response so we can directly fetch
// from JS.
// const proxyUrl = 'https://cors-anywhere.herokuapp.com/'

// @ts-ignore
window.braveStorybookUnpadUrl = async function UnpadUrl (paddedUrl: string, mimeType = 'image/jpg'): Promise<string> {
  // const response = await fetch(proxyUrl + paddedUrl)
  // const blob = await response.blob();
  // // @ts-ignore (Blob.arrayBuffer does exist)
  // const buffer = await blob.arrayBuffer()
  // const dataUrl = await getUnpaddedAsDataUrl(buffer, mimeType)
  // return dataUrl
}

function ThemeProvider ({ story }: any) {
  return (
    <BraveCoreThemeProvider
      dark={DarkTheme}
      light={Theme}
      initialThemeType={select(
        'Theme',
        { ['Light']: 'Light', ['Dark']: 'Dark' },
        'Light'
      )}
    >
    {story}
    </BraveCoreThemeProvider>
  )
}

function StoreProvider ({ story }: any) {
  return (
    <ReduxProvider store={store}>
     {story}
    </ReduxProvider>
  )
}

function dismissBraveTodayIntroCard () {
  console.log('brave today intro card dismissed')
}

storiesOf('New Tab/Containers', module)
  .addDecorator(withKnobs)
  .addDecorator(story => <StoreProvider story={story()} />)
  .addDecorator(story => <ThemeProvider story={story()} />)
  .add('Default', () => {
    const doNothing = (value: boolean) => value
    const state = store.getState()
    const newTabData = getNewTabData(state.newTabData)
    const gridSitesData = getGridSitesData(state.gridSitesData)
    const todayState = getTodayState()
    return (
      <NewTabPage
        newTabData={newTabData}
        todayData={todayState}
        gridSitesData={gridSitesData}
        actions={getActions()}
        saveShowBackgroundImage={doNothing}
        saveShowStats={doNothing}
        saveShowToday={doNothing}
        saveShowRewards={doNothing}
        saveShowBinance={doNothing}
        saveShowTogether={doNothing}
        saveShowGemini={doNothing}
        saveShowCryptoDotCom={doNothing}
        saveBrandedWallpaperOptIn={doNothing}
        onReadBraveTodayIntroCard={dismissBraveTodayIntroCard}
      />
    )
  })
