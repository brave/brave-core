/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import data from './page/fakeData'

// Components
import ResetSyncModal from './modals/resetSync'
import DeviceTypeModal from './modals/deviceType'
import ScanCodeModal from './modals/scanCode'
import AddNewChainNoCameraModal from './modals/addNewChainNoCamera'
import EnterSyncCodeModal from './modals/enterSyncCode'
import ViewSyncCodeModal from './modals/viewSyncCode'
import RemoveMainDeviceModal from './modals/removeMainDevice'
import RemoveOtherDeviceModal from './modals/removeOtherDevice'

const doNothing = () => {
  console.log('nothing')
}

storiesOf('Feature Components/Sync/Popups and Modals', module)
  .add('Reset Sync', () => <ResetSyncModal onClose={doNothing} mainDeviceName={data.device1.name} />)
  .add('Device Type', () => <DeviceTypeModal onClose={doNothing} />)
  .add('Scan Code', () => <ScanCodeModal onClose={doNothing} />)
  .add('Add New Chain (no camera)', () => <AddNewChainNoCameraModal onClose={doNothing} />)
  .add('Enter Sync Code', () => <EnterSyncCodeModal onClose={doNothing} />)
  .add('View Sync Code', () => <ViewSyncCodeModal onClose={doNothing} />)
  .add('Remove Main Device', () => <RemoveMainDeviceModal onClose={doNothing} mainDeviceName={data.device1.name} />)
  .add('Remove Other Device', () => <RemoveOtherDeviceModal onClose={doNothing} otherDeviceName={data.device2.name} />)
