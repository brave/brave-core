// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

type SessionId = string | null
type Header = { name: string, value: string }

const twitterAuthHeaderNames = [
  'authorization',
  'x-csrf-token',
  'x-guest-token'
]
const authTokenCookieRegex = /[; ]_twitter_sess=([^\s;]*)/

let twitterAuthHeaders = {}
let lastSessionId: SessionId = null

function readTwitterSessionCookie (cookiesString: string): SessionId {
  const match = cookiesString.match(authTokenCookieRegex)
  if (match) {
    return unescape(match[1])
  }
  return null
}

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'getTwitterAPICredentials': {
      sendResponse(twitterAuthHeaders)
      break
    }
    default:
      break
  }
})

// Grab auth headers from twitter's normal requests
chrome.webRequest.onSendHeaders.addListener(
  // Listener
  function ({ requestHeaders }) {
    if (requestHeaders) {
      for (const header of requestHeaders) {
        // Parse cookies for session id
        if (header.name === 'Cookie') {
          let currentSessionId = readTwitterSessionCookie(header.value as string)
          const hasAuthChanged = (currentSessionId !== lastSessionId)
          if (hasAuthChanged) {
            // clear cached auth data when session changes
            lastSessionId = currentSessionId
            twitterAuthHeaders = { }
          }
        } else if (twitterAuthHeaderNames.includes(header.name) || header.name.startsWith('x-twitter-')) {
          twitterAuthHeaders[header.name] = header.value
        }
      }
    }
  },
  // Filters
  {
    urls: [
      'https://api.twitter.com/1.1/*'
    ]
  },
  // Extra
  [
    'requestHeaders',
    'extraHeaders'  // need cookies
  ])
