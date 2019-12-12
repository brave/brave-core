/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const locale: any = {
  // Shared
  done: 'Done',
  remove: 'Remove',
  copied: 'Copied!',
  wordCount: 'Word Count:',
  ok: 'Ok',
  cancel: 'Cancel',
  thisSyncChain: 'from Sync Chain',
  lookingForDevice: 'Looking for device',
  // Enabled Content
  braveSync: 'Brave Sync',
  syncChainDevices:
    'The device list below includes all devices in your sync chain. Each device can be managed from any other device.',
  deviceName: 'device name',
  addedOn: 'Date Added',
  addDevice: 'Add New Device',
  viewSyncCode: 'View Sync Code',
  syncSettings: 'Brave Sync Settings',
  settings: 'Settings',
  syncSettingsDescription:
    'Manage the content you would like to sync between devices. These settings only affect this device.',
  bookmarks: 'Bookmarks',
  savedSiteSettings: 'Saved Site Settings',
  browsingHistory: 'Browsing History',
  leaveSyncChain: 'Delete Sync Chain',
  // Disabled Content
  syncTitle: 'Brave Sync Setup',
  syncDescription:
    'To start, you will need Brave installed on all the devices you plan to sync. To chain them together, start a sync chain that you will use to securely link all of your devices together.',
  startSyncChain: 'Start a new Sync Chain',
  enterSyncChainCode: 'I have a Sync Code',
  confirmSyncCode: 'Confirm Sync Code',
  // [modal] Enter Sync Code
  enterSyncCode: 'Enter Your Sync Chain Code Words',
  enterSyncCodeDescription:
    'Type your supplied sync chain code words into the form below.',
  confirmCode: 'Confirm Sync Code',
  invalidCode: 'Invalid sync code.',
  tryAgain: 'Please try again.',
  // [modal] Remove Main Device
  thisDeviceRemovalDescription:
    'Local device data will remain intact on all devices. Other devices in this sync chain will remain synced.',
  // [modal] Remove Other Device
  otherDeviceRemovalDescription:
    'Removing the device from the Sync Chain will not clear previously synced data from the device.',
  // [modal] Reset Sync
  warning: 'Warning!',
  removing: 'Removing',
  deleteSyncChain: 'will delete the sync chain.',
  deleteSyncDescription:
    'Data currently synced will be retained but all data in Brave’s Sync cache will be deleted. You will need to start a new sync chain to sync device data again.',
  deleteSyncChainButton: 'Delete Sync Chain',
  areYouSure: 'Are you sure?',
  // [modal] Add New Chain
  scanThisCode: 'Sync Chain QR Code',
  scanThisCodeHowTo:
    'On your mobile device, navigate to Brave Sync in the Settings panel and click the button "Scan or enter sync code". Use your camera to scan the QR Code below.',
  viewCodeWords: 'View Code Words',
  // [modal] Add New Chain (Camera Option)
  enterThisCode: 'Using Brave on your computer, enter this code:',
  mobileEnterThisCode: 'Using Brave on your mobile device, enter this code:',
  syncChainCodeHowTo: 'Go to: Brave Settings > Sync > Enter a Sync Chain Code',
  // [modal] Add New Chain (no Camera)
  // [modal] View Sync Code
  chainCode: 'Sync Chain Code',
  chainCodeDescriptionPartial1:
    'On your target computer, navigate to Brave Sync in settings and click the button',
  chainCodeDescriptionPartial2: '“I have a Sync Code”.',
  chainCodeDescriptionPartial3: 'Enter the sync chain code words shown below.',
  useCameraInstead: 'Use camera instead',
  qrCode: 'View QR Code',
  // [modal] Choose Device Type
  letsSync: 'Choose Device Type',
  chooseDeviceType: 'Choose the type of device you would like to sync to.',
  phoneTablet: 'Tablet or Phone',
  computer: 'Computer'
}

export default locale

export const getLocale = (word: string) => locale[word]
