/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import { withKnobs, boolean } from '@storybook/addon-knobs'

// Components
import NewPrivateTab from './private/index'

storiesOf('Feature Components/New Tab', module)
  .addDecorator(withKnobs)
  .add('Private', () => <NewPrivateTab isTor={boolean('Enable Tor?', false)} />)
