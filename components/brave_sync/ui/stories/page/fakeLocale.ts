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
  syncChainDevices: 'The device list below includes all devices in your sync chain. Each device can be managed from any other device.',
  deviceName: 'device name',
  addedOn: 'Date Added',
  addDevice: 'Add New Device',
  viewSyncCode: 'View Sync Code',
  syncSettings: 'Brave Sync Settings',
  syncSettingsDescription: 'Manage what information you would like to sync between devices. These settings only effect this device.',
  bookmarks: 'Bookmarks',
  savedSiteSettings: 'Saved Site Settings',
  browsingHistory: 'Browsing History',
  leaveSyncChain: 'Leave Sync Chain',
  // Disabled Content
  syncTitle: 'Brave Sync Setup',
  syncDescription: 'To start, you will need Brave installed on all the devices you plan to sync. To chain them together, start a sync chain that you will use to securely link all of your devices together.',
  startSyncChain: 'Start a New Sync Chain',
  enterSyncChainCode: 'I Have a Sync Code',
  confirmSyncCode: 'Confirm Sync Code',
  // [modal] Enter Sync Code
  enterSyncCode: 'Enter Your Sync Chain Code Words',
  enterSyncCodeDescription: 'Type your supplied sync chain code words into the form below.',
  confirmCode: 'Confirm Sync Code',
  invalidCode: 'Invalid sync code.',
  tryAgain: 'Please try again.',
  // [modal] Remove Main Device
  thisDeviceRemovalDescription: 'Local device data will remain intact on all devices. Other devices in this sync chain will remain synced.',
  // [modal] Remove Other Device
  otherDeviceRemovalDescription: 'Removing the device from the Sync Chain will not clear previously synced data from the device.',
  // [modal] Reset Sync
  warning: 'Warning!',
  removing: 'Removing',
  deleteSyncChain: 'will delete the sync chain.',
  deleteSyncDescription: 'Data currently synced will be retained but all data in Brave’s Sync cache will be deleted. You will need to start a new sync chain to sync device data again.',
  deleteSyncChainButton: 'Delete Sync Chain',
  areYouSure: 'Are you sure?',
  // [modal] Add New Chain
  scanThisCode: 'Sync Chain QR Code',
  scanThisCodeHowTo: 'On your mobile device, navigate to Brave Sync in the Settings panel and click the button “Scan Sync Code”. Use your camera to scan the QR Code below.',
  viewCodeWords: 'View Code Words',
  // [modal] Add New Chain (Camera Option)
  enterThisCode: 'Now, using Brave on your computer, enter this code:',
  mobileEnterThisCode: 'Now, using Brave on your mobile device, enter this code:',
  syncChainCodeHowTo: 'Go to: Brave Settings > Sync > Enter a Sync Chain Code',
  // [modal] Add New Chain (no Camera)
  chainCode: 'Sync Chain Code',
  chainCodeDescription: 'On your target computer, navigate to Brave Sync in settings and click the button “I Have a Sync Code”. Type in the below sync chains code words.',
  useCameraInstead: 'Use camera instead',
  // [modal] Choose Device Type
  letsSync: 'Choose Device Type',
  chooseDeviceType: 'Choose the type of device you would like to sync to.',
  phoneTablet: 'Tablet or Phone',
  computer: 'Computer',
  // [modal] View Sync Code
  qrCode: 'View QR Code'
}

export default locale

export const getLocale = (word: string) => locale[word]
