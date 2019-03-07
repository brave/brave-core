/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
// @ts-ignore
import { withKnobs, boolean, number } from '@storybook/addon-knobs'
import { withState } from '@dump247/storybook-state'
const favicon = require('../../assets/img/fake_favicon.png')

import 'emptykit.css'

// Components
import Shields from './index'

storiesOf('Feature Components/Shields', module)
  .addDecorator(withKnobs)
  .add('Enabled', withState({ enabled: true }, (store) => {
    const fakeOnChange = () => {
      store.set({ enabled: !store.state.enabled })
    }
    return (
      <Shields
        favicon={favicon}
        hostname={'buzzfeed.com'}
        enabled={boolean('Enabled?', store.state.enabled)}
        adsTrackersBlocked={number('3rd-party trackers blocked', 80) || 0}
        httpsUpgrades={number('Connections upgraded to HTTPS', 0) || 0}
        scriptsBlocked={number('Scripts blocked', 11) || 0}
        fingerprintingBlocked={number('3rd-party device recognition blocked', 0) || 0}
        fakeOnChange={fakeOnChange}
      />
    )
  }))
