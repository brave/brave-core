/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Provider as ReduxProvider } from 'react-redux'
import { storiesOf } from '@storybook/react'
import { withKnobs, select } from '@storybook/addon-knobs/react'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'

// Components
import NewTabPage from '../containers/newTab'
import * as newTabActions from '../actions/new_tab_actions'
import * as gridSitesActions from '../actions/grid_sites_actions'
import * as rewardsActions from '../actions/rewards_actions'
import * as binanceActions from '../actions/binance_actions'
import * as geminiActions from '../actions/gemini_actions'
import * as bitcoinDotComActions from '../actions/bitcoin_dot_com_actions'
import * as cryptoDotComActions from '../actions/cryptoDotCom_actions'
import * as stackWidgetActions from '../actions/stack_widget_actions'
import * as todayActions from '../actions/today_actions'

import store from '../store'
import { getNewTabData, getGridSitesData } from './default/data/storybookState'
import getTodayState from './default/data/todayStorybookState'
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

export default function Provider ({ story }: any) {
  return (
    <ReduxProvider store={store}>
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
    </ReduxProvider>
  )
}

storiesOf('New Tab/Containers', module)
  .addDecorator(withKnobs)
  .addDecorator(story => <Provider story={story()} />)
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
        actions={Object.assign({}, newTabActions, stackWidgetActions, gridSitesActions, rewardsActions, binanceActions, geminiActions, bitcoinDotComActions, cryptoDotComActions, todayActions)}
        saveShowBackgroundImage={doNothing}
        saveShowStats={doNothing}
        saveShowToday={doNothing}
        saveShowRewards={doNothing}
        saveShowBinance={doNothing}
        saveShowTogether={doNothing}
        saveShowAddCard={doNothing}
        saveShowGemini={doNothing}
        saveShowBitcoinDotCom={doNothing}
        saveShowCryptoDotCom={doNothing}
        saveBrandedWallpaperOptIn={doNothing}
      />
    )
  })
