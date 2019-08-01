// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

var soundcloudClientId : string

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  //console.log('soundcloudAuth.ts')
  //console.log(typeof msg == 'string' ? msg : JSON.stringify(msg))
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'getSoundCloudClientId': {
      sendResponse(soundcloudClientId)
      break
    }
    default:
      break
  }
})

// Grab auth headers from soundcloud's normal requests
chrome.webRequest.onBeforeRequest.addListener(
  function ({ url }) {
    if(url) {
      let chunks = url.split('/')
      const queryString = chunks[chunks.length - 1]
      let queryParams = new URLSearchParams(queryString)
      var newId = queryParams.get('client_id')
      if(newId) {
        soundcloudClientId = newId
      }
    }
  },
   // Filters
  { urls: [
      'https://api-v2.soundcloud.com/*'
    ]
  })