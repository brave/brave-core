// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { initializeDetector } from './creator_detection'
import { log, pollFor, absoluteURL } from './helpers'

initializeDetector(() => {

  function getIdFromPerson(person: any) {
    if (!person) {
      return ''
    }
    if (person.identifier) {
      return String(person.identifier)
    }
    const { potentialAction } = person
    if (!potentialAction) {
      log.error('Missing "potentialAction" in Person')
      return ''
    }
    const { target } = potentialAction
    if (!target || typeof target !== 'string') {
      log.error('Missing "potentialAction.target" in Person')
      return ''
    }
    const match = target.match(/\/users\/([^\/]+)/i)
    if (!match) {
      log.error('Unable to read ID from Person')
      return ''
    }
    return match[1]
  }

  function getUserFromPerson(person: any) {
    const id = getIdFromPerson(person)
    if (!id) {
      return null
    }
    return {
      id: `vimeo#channel:${id}`,
      name: String(person.name || ''),
      url: absoluteURL(person.url || location.href),
      imageURL: String(person.image || '')
    }
  }

  function getUserFromVideoData(video: any) {
    if (!video) {
      return null
    }
    const { author } = video
    if (!author) {
      log.error('Missing "author" in VideoObject')
      return null
    }
    return getUserFromPerson(author)
  }

  function getUserFromMetadata(json: string) {
    let data: any
    try {
      data = JSON.parse(json)
    } catch (err) {
      log.error('Invalid JSON in metadata script')
      return null
    }
    if (!Array.isArray(data)) {
      log.error('Invalid metadata object: expected an array')
      return null
    }
    for (const item of data) {
      switch (item['@type']) {
        case 'Person': {
          const user = getUserFromPerson(item)
          if (user) {
            return user
          }
        }
        case 'VideoObject': {
          const user = getUserFromVideoData(item)
          if (user) {
            return user
          }
        }
      }
    }
    log.info('Unable to find user in metadata scripts')
    return null
  }

  const metadataScriptSelector = 'script[type="application/ld+json"]'

  function parseMetadataScript() {
    const elems = document.querySelectorAll<HTMLElement>(metadataScriptSelector)
    for (const elem of elems) {
      const user = getUserFromMetadata(elem.innerText)
      if (user) {
        return user
      }
    }
    return null
  }

  function hasMetadataScript() {
    return Boolean(document.querySelector(metadataScriptSelector))
  }

  return async () => {
    await pollFor(hasMetadataScript, {
      name: 'metadata',
      interval: 500,
      timeout: 6000
    })

    return parseMetadataScript()
  }

})
