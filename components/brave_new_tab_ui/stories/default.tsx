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
import * as actions from '../actions/new_tab_actions'
import store from '../store'
import { getNewTabData } from './default/data/storybookState'

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
    return (
      <NewTabPage
        newTabData={getNewTabData(store.getState().newTabData as NewTab.State)}
        actions={actions}
        saveShowBackgroundImage={doNothing}
        saveShowClock={doNothing}
        saveShowTopSites={doNothing}
        saveShowStats={doNothing}
        saveShowRewards={doNothing}
        saveShowBinance={doNothing}
        saveBrandedWallpaperOptIn={doNothing}
      />
    )
  })
