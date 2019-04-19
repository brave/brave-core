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
  blockedScriptsResouces: {
    'https://imasdk.googleapis.com/js/sdkloader/ima3.js': {
      actuallyBlocked: true,
      willBlock: true,
      userInteracted: true
    },
    'https://scripts.dailymail.co.uk/rta2/v-0.37.min.js': {
      actuallyBlocked: true,
      willBlock: true,
      userInteracted: false
    },
    'https://scripts.dailymail.co.uk/static/gunther/17.7.2/async_bundle--.js': {
      actuallyBlocked: true,
      willBlock: true,
      userInteracted: false
    },
    'https://scripts.dailymail.co.uk/static/mol-adverts/1281/mol-adverts.js': {
      actuallyBlocked: true,
      willBlock: true,
      userInteracted: false
    },
    'https://scripts.dailymail.co.uk/static/mol-fe/static/mol-fe-async-bundle//5.8.5/channelDefer.js': {
      actuallyBlocked: true,
      willBlock: true,
      userInteracted: false
    },
    'https://scripts.dailymail.co.uk/static/mol-fe/static/mol-fe-fff/1.3.12/scripts/fff.js': {
      actuallyBlocked: true,
      willBlock: true,
      userInteracted: false
    },
    'https://scripts.dailymail.co.uk/static/mol-fe/static/mol-fe-sync-bundle/3.38.1/desktop.js': {
      actuallyBlocked: true,
      willBlock: true,
      userInteracted: false
    },
    'https://scripts.dailymail.co.uk/static/videoplayer//5.14.0/scripts/mol-fe-videoplayer.min.js': {
      actuallyBlocked: false,
      willBlock: false,
      userInteracted: true
    },
    'https://www.googletagservices.com/tag/js/gpt.js': {
      actuallyBlocked: false,
      willBlock: false,
      userInteracted: false
    }
  },
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
