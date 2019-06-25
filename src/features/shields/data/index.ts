/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const dummyData = {
  thirdPartyCookiesBlocked: '2',
  thirdPartyScriptsBlocked: '2',
  thirdPartyDeviceRecognitionBlocked: '2',
  pishingMalwareBlocked: '2',
  connectionsEncrypted: '2',
  noScriptsResouces: {
    'https://imasdk.googleapis.com/js/sdkloader/ima3.js': {
      actuallyBlocked: true,
      willBlock: true,
      userInteracted: true
    },
    'https://scripts.dailymail.co.uk/rta2/v-0.37.min.js': {
      actuallyBlocked: true,
      willBlock: true,
      userInteracted: true
    }
  },
  otherBlockedResources: [
    'https://aaaa.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://bbbb.com/abcdefghijklmnopqrstuwxyz/123456789'
  ]
}
