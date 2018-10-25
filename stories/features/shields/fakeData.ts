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
    { name: 'https://aaaa.com/123456789123456789123456789, ', blocked: false, hasUserInput: false },
    { name: 'https://bbbb.com/123456789123456789123456789, ', blocked: false, hasUserInput: false },
    { name: 'https://cccc.com/123456789123456789123456789, ', blocked: false, hasUserInput: true },
    { name: 'https://dddd.com/123456789123456789123456789, ', blocked: false, hasUserInput: false },
    { name: 'https://eeee.com/123456789123456789123456789, ', blocked: false, hasUserInput: false },
    { name: 'https://ffff.com/123456789123456789123456789, ', blocked: false, hasUserInput: false },
    { name: 'https://gggg.com/123456789123456789123456789, ', blocked: true, hasUserInput: false },
    { name: 'https://hhhh.com/123456789123456789123456789, ', blocked: true, hasUserInput: true },
    { name: 'https://iiii.com/123456789123456789123456789, ', blocked: true, hasUserInput: false },
    { name: 'https://jjjj.com/123456789123456789123456789, ', blocked: true, hasUserInput: false },
    { name: 'https://kkkk.com/123456789123456789123456789, ', blocked: true, hasUserInput: false },
    { name: 'https://llll.com/123456789123456789123456789, ', blocked: true, hasUserInput: false }
  ],

  blockedFakeResources: [
    'https://aaaa.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://bbbb.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://cccc.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://dddd.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://eeee.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://ffff.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://gggg.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://hhhh.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://iiii.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://jjjj.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://kkkk.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://llll.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://mmmm.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://nnnn.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://oooo.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://pppp.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://qqqq.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://rrrr.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://ssss.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://tttt.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://uuuu.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://vvvv.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://wwww.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://xxxx.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://yyyy.com/abcdefghijklmnopqrstuwxyz/123456789',
    'https://zzzz.com/abcdefghijklmnopqrstuwxyz/123456789'
  ]
}

export default data
