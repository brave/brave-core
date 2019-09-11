/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import shieldsLightTheme from '../theme/shields-light'
import shieldsDarkTheme from '../theme/shields-dark'
import { withThemesProvider } from 'storybook-addon-styled-component-theme'
// @ts-ignore
import { withKnobs, boolean, number } from '@storybook/addon-knobs'
import { withState } from '@dump247/storybook-state'
import favicon from './images/fake_favicon.png'

// Components
import Shields from './index'
import ShieldsReadOnlyView from './components/readOnlyView'

// Themes
const themes = [shieldsLightTheme, shieldsDarkTheme]

storiesOf('Shields', module)
  .addDecorator(withThemesProvider(themes))
  .addDecorator(withKnobs)
  .add('Panel', withState({ enabled: true, advancedView: false, readOnlyView: false, firstAccess: true }, (store) => {
    const fakeOnChangeShieldsEnabled = () => {
      store.set({ enabled: !store.state.enabled })
    }
    const fakeOnChangeAdvancedView = () => {
      store.set({ advancedView: !store.state.advancedView })
    }
    const fakeOnChangeReadOnlyView = () => {
      store.set({ readOnlyView: !store.state.readOnlyView })
    }
    const fakeToggleFirstAccess = () => {
      store.set({ firstAccess: !store.state.firstAccess })
    }
    return (
      <div style={{ margin: '120px' }}>
        {
          store.state.readOnlyView
          ? (
            <ShieldsReadOnlyView
              favicon={favicon}
              hostname={'buzzfeed.com'}
              onClose={fakeOnChangeReadOnlyView}
            />
          ) : (
            <Shields
              enabled={boolean('Enabled?', store.state.enabled)}
              firstAccess={boolean('First Access?', store.state.firstAccess)}
              favicon={favicon}
              hostname={'buzzfeed.com'}
              advancedView={boolean('Show advanced view?', store.state.advancedView)}
              adsTrackersBlocked={number('3rd-party trackers blocked', 80) || 0}
              httpsUpgrades={number('Connections upgraded to HTTPS', 0) || 0}
              scriptsBlocked={number('Scripts blocked', 11) || 0}
              fingerprintingBlocked={number('3rd-party device recognition blocked', 0) || 0}
              fakeOnChangeShieldsEnabled={fakeOnChangeShieldsEnabled}
              fakeOnChangeAdvancedView={fakeOnChangeAdvancedView}
              fakeOnChangeReadOnlyView={fakeOnChangeReadOnlyView}
              fakeToggleFirstAccess={fakeToggleFirstAccess}
            />
          )
        }
    </div>
    )
  }))
