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
import store from '../store'
import { getNewTabData, getGridSitesData } from './default/data/storybookState'

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
    const newTabData = getNewTabData(store.getState().newTabData)
    const gridSitesData = getGridSitesData(store.getState().gridSitesData)
    return (
      <NewTabPage
        newTabData={newTabData}
        gridSitesData={gridSitesData}
        actions={Object.assign({}, newTabActions, gridSitesActions, rewardsActions, binanceActions, geminiActions)}
        saveShowBackgroundImage={doNothing}
        saveShowClock={doNothing}
        saveShowTopSites={doNothing}
        saveShowStats={doNothing}
        saveShowRewards={doNothing}
        saveShowBinance={doNothing}
        saveShowTogether={doNothing}
        saveShowAddCard={doNothing}
        saveShowGemini={doNothing}
        saveBrandedWallpaperOptIn={doNothing}
      />
    )
  })
