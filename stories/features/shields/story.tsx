/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
// @ts-ignore
import centered from '@storybook/addon-centered/dist'
import { withKnobs, boolean } from '@storybook/addon-knobs'
import { withState } from '@dump247/storybook-state'

// Components
import Shields from './index'

storiesOf('Feature Components/Shields', module)
  // .addDecorator(centered)
  .addDecorator(withKnobs)
  .add('Enabled', withState({ enabled: false }, (store) => {
    const fakeOnChange = () => {
      store.set({ enabled: !store.state.enabled })
    }
    return <Shields fakeOnChange={fakeOnChange} enabled={boolean('Enabled?', store.state.enabled)} />
  }))
  .add('Disabled', withState({ enabled: false }, (store) => {
    const fakeOnChange = () => {
      store.set({ enabled: !store.state.enabled })
    }
    return <Shields fakeOnChange={fakeOnChange} enabled={boolean('Enabled?', store.state.enabled)} />
  }))
