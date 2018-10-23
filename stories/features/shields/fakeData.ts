/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const data = {
  thirdPartyCookiesBlocked: '33',
  thirdPartyScriptsBlocked: '11',
  thirdPartyDeviceRecognitionBlocked: '10',
  pishingMalwareBlocked: '23',
  connectionsEncrypted: '1',
  totalBlocked: '300',
  siteName: 'buzzfeed.com',
  popupsBlocked: '2',
  imagesBlocked: '33',
  totalAdsTrackersBlocked: '100',
  blockedScriptsResouces: [
    { name: 'https://aaaa/com/123456789123456789123456789, ', blocked: false },
    { name: 'https://bbbb/com/123456789123456789123456789, ', blocked: false },
    { name: 'https://cccc/com/123456789123456789123456789, ', blocked: false },
    { name: 'https://dddd/com/123456789123456789123456789, ', blocked: false },
    { name: 'https://eeee/com/123456789123456789123456789, ', blocked: false },
    { name: 'https://ffff/com/123456789123456789123456789, ', blocked: false },
    { name: 'https://gggg/com/123456789123456789123456789, ', blocked: true },
    { name: 'https://hhhh/com/123456789123456789123456789, ', blocked: true },
    { name: 'https://iiii/com/123456789123456789123456789, ', blocked: true },
    { name: 'https://jjjj/com/123456789123456789123456789, ', blocked: true },
    { name: 'https://kkkk/com/123456789123456789123456789, ', blocked: true },
    { name: 'https://llll/com/123456789123456789123456789, ', blocked: true }
  ],

  blockedFakeResources: [
    'https://aaaa/com',
    'https://bbbb/com',
    'https://cccc/com',
    'https://dddd/com',
    'https://eeee/com',
    'https://ffff/com',
    'https://gggg/com',
    'https://hhhh/com',
    'https://iiii/com',
    'https://jjjj/com',
    'https://kkkk/com',
    'https://llll/com',
    'https://mmmm/com',
    'https://nnnn/com',
    'https://oooo/com',
    'https://pppp/com',
    'https://qqqq/com',
    'https://rrrr/com',
    'https://ssss/com',
    'https://tttt/com',
    'https://uuuu/com',
    'https://vvvv/com',
    'https://wwww/com',
    'https://xxxx/com',
    'https://yyyy/com',
    'https://zzzz/com'
  ]
}

export default data
