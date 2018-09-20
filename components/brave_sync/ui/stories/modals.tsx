/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'

// Components
import ExistingSyncCodeModal from './modals/existingSyncCode'
import NewToSyncModal from './modals/newToSync'
import ResetSyncModal from './modals/resetSync'
import SyncANewDeviceModal from './modals/syncNewDevice'

const doNothing = () => {
  console.log('nothing')
}

storiesOf('Feature Components/Sync/Popups and Modals', module)
  .add('Existing Sync Code', () => <ExistingSyncCodeModal onClose={doNothing} />)
  .add('New to Sync', () => <NewToSyncModal onClose={doNothing} />)
  .add('Reset Sync', () => <ResetSyncModal onClose={doNothing} />)
  .add('Sync New Device', () => <SyncANewDeviceModal onClose={doNothing} />)
