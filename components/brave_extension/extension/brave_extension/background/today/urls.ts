// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// This won't change unless browser restarted (provided via flag), so it's ok
// to store as a global.
let hostnameCache: string

function getHostname (): Promise<string> {
  if (hostnameCache) {
    return Promise.resolve(hostnameCache)
  }
  return new Promise(resolve => {
    chrome.braveToday.getHostname((newHostname) => {
      hostnameCache = newHostname
      resolve(hostnameCache)
    })
  })
}

export async function getFeedUrl () {
  const hostname = await getHostname()
  return `https://${hostname}/brave-today/feed.json`
}

export async function getSourcesUrl () {
  const hostname = await getHostname()
  return `https://${hostname}/sources.json`
}

// Always get the hostname at startup, it's cheap
getHostname().then(hostname => {
  hostnameCache = hostname
  console.debug('Brave Today hostname', hostname)
}).catch(reason => {
  console.error('Brave Today could not fetch hostname.', reason)
})
