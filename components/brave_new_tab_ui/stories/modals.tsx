/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'

// Components
import DuckDuckGoModal from '../newTab/private/modals/duckDuckGoModal'
import PrivateWindowsModal from '../newTab/private/modals/privateWindowsModal'
import PrivateWindowsWithTorModal from '../newTab/private/modals/privateWindowsWithTorModal'
import TorInBraveModal from '../newTab/private/modals/torInBraveModal'

const doNothing = () => {
  console.log('nothing')
}

storiesOf('Feature Components/New Tab/Private/Popups and Modals', module)
  .add('Learn more DDG Modal', () => <DuckDuckGoModal onClose={doNothing} />)
  .add('Learn more Private Window Modal', () => <PrivateWindowsModal onClose={doNothing} />)
  .add('Learn more Private Window with Tor Modal', () => <PrivateWindowsWithTorModal onClose={doNothing} />)
  .add('Learn more about Tor Modal', () => <TorInBraveModal onClose={doNothing} />)
