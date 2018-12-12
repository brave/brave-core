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
  thisSyncChain: 'from this sync chain',
  // Enabled Content
  braveSync: 'Brave Sync',
  syncChainDevices: 'Devices in your sync chain',
  deviceName: 'device name',
  addedOn: 'added on',
  addDevice: 'add device',
  viewSyncCode: 'view sync code',
  dataToSync: 'Data to sync from',
  bookmarks: 'Bookmarks',
  savedSiteSettings: 'Saved Site Settings',
  browsingHistory: 'Browsing History',
  leaveSyncChain: 'Leave Sync Chain',
  // Disabled Content
  syncTitle: 'Brave Sync allows you to sync bookmarks data privately between your Brave Browsers on various devices.',
  syncDescription: 'When you start a new sync chain, a new sync code is created. You will use this same code across your devices to link them together.',
  startSyncChain: 'Start a new sync chain',
  enterSyncChainCode: 'Enter a sync chain code',
  confirmSyncCode: 'Confirm Sync Code',
  // [modal] Enter Sync Code
  enterSyncCode: 'Enter a sync code',
  enterSyncCodeDescription: 'Go to Brave Settings > Sync > Display sync code',
  confirmCode: 'Confirm Sync Code',
  invalidCode: 'Invalid sync code.',
  tryAgain: 'Please try again.',
  // [modal] Remove Main Device
  thisDeviceRemovalDescription: 'Local device data will remain intact on all devices. Other devices in this sync chain will remain synced.',
  joinSyncChain: 'To join a sync chain again, choose “Enter a sync chain code”.',
  // [modal] Remove Other Device
  otherDeviceRemovalDescription: 'Note: Removing this device from this sync chain does not clear previously synced data from the device.',
  // [modal] Reset Sync
  warning: 'Warning!',
  removing: 'Removing',
  deleteSyncChain: 'will delete this sync chain.',
  deleteSyncDescription: 'Your local data on this device will be retained but all data in Brave’s Sync cache will be deleted.',
  startSyncChainHowTo: 'You can start a new sync chain or sync with a new device by entering its sync chain code.',
  areYouSure: 'Are you sure?',
  // [modal] Add New Chain
  scanThisCode: 'Now, using Brave on your mobile device, scan this code.',
  scanThisCodeHowTo: 'Go to: Brave Settings > Sync > Scan Sync Code',
  enterCodeWordsInstead: 'Enter code words instead',
  previous: '< Previous',
  lookingForDevice: 'Looking for device',
  // [modal] Add New Chain (Camera Option)
  // [modal] Add New Chain (no Camera)
  enterThisCode: 'Now, using Brave on your computer, enter this code:',
  mobileEnterThisCode: 'Now, using Brave on your mobile device, enter this code:',
  syncChainCodeHowTo: 'Go to: Brave Settings > Sync > Enter a Sync Chain Code',
  useCameraInstead: 'Use camera instead',
  // [modal] Choose Device Type
  letsSync: 'Let’s sync a new device with',
  chooseDeviceType: 'Choose a device type:',
  phoneTablet: 'Phone/Tablet',
  computer: 'Computer',
  // [modal] View Sync Code
  qrCode: 'QR Code',
  wordCode: 'Word Code',
  privateKey: 'This is a private key. If you share it, your private data may be compromised.'
}

export default locale

export const getLocale = (word: string) => locale[word]
