/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Util
import rewardsPanelActions from './actions/rewardsPanelActions'

interface GreaselionErrorResponse {
  errorMessage: string
}

interface SavePublisherVisitResponse {
  url: string
  publisherKey: string
  publisherName: string
  mediaKey?: string
  favIconUrl?: string
}

interface GreaselionResponse {
  type: 'GreaselionError' | 'SavePublisherVisit'
  mediaType: string
  data: GreaselionErrorResponse | SavePublisherVisitResponse | null
}

interface MediaDurationHandlerRegistrationRequest {
  urlRegex: string
}

interface MediaDurationMetadataResponse {
  mediaKey: string
  duration: number
}

// Media duration handler registration sentinel
interface MediaDurationHandlerRegistration {
  registered: boolean
}

// Maps a tabId to a registration sentinel
const mediaDurationHandlerRegistrations = new Map<number, MediaDurationHandlerRegistration>()

// Maps a tabId to a specific Greaselion port
const greaselionPorts = new Map<number, chrome.runtime.Port>()

// Maps a media key to a specific publisher
const publisherKeys = new Map<string, string>()

const handleGreaselionErrorResponse = (tabId: number, mediaType: string, data: GreaselionErrorResponse) => {
  console.error(`Greaselion error: ${data.errorMessage}`)
}

const handleMediaDurationMetadataResponse = (tabId: number, mediaType: string, data: MediaDurationMetadataResponse) => {
  const publisherKey = publisherKeys[data.mediaKey]
  if (!publisherKey) {
    console.error(`Failed to handle media duration metadata response: missing publisher key for media key ${data.mediaKey}`)
    return
  }

  chrome.braveRewards.updateMediaDuration(tabId, publisherKey, data.duration)
}

const handleMediaDurationHandlerRegistrationRequest = (tabId: number, mediaType: string, data: MediaDurationHandlerRegistrationRequest) => {
  // If we already registered a handler for this tab, exit early
  if (mediaDurationHandlerRegistrations[tabId] && mediaDurationHandlerRegistrations[tabId].registered) {
    return
  }

  // Mark this tab as registered
  mediaDurationHandlerRegistrations[tabId] = {
    registered: true
  }

  chrome.webRequest.onCompleted.addListener(
    // Listener
    function (details) {
      const port = greaselionPorts.get(tabId)
      if (!port) {
        return
      }
      port.postMessage({
        type: 'MediaDurationMetadataRequest',
        url: details.url
      })
    },
    // Filters
    {
      types: [
        'image',
        'media',
        'script',
        'xmlhttprequest'
      ],
      urls: [
        data.urlRegex
      ]
    })
}

const getPublisherPanelInfo = (tabId: number, publisherKey: string) => {
  chrome.braveRewards.getPublisherPanelInfo(
    publisherKey, (result: RewardsExtension.Result, info?: RewardsExtension.Publisher) => {
      if (result === 0 && info) {
        rewardsPanelActions.onPublisherData(tabId, info)
      }
      return
    })
}

const savePublisherInfo = (tabId: number, mediaType: string, url: string, publisherKey: string, publisherName: string, favIconUrl: string) => {
  chrome.braveRewards.savePublisherInfo(
    tabId,
    mediaType,
    url,
    publisherKey,
    publisherName,
    favIconUrl,
    (result: RewardsExtension.Result) => {
      if (result !== 0) {
        console.error(`Failed to save publisher info for ${publisherKey}, result is ${result}`)
        return
      }
    })
}

const handleSavePublisherVisitResponse = (tabId: number, mediaType: string, data: SavePublisherVisitResponse) => {
  if (!data.publisherKey) {
    console.error('Failed to handle publisher visit: missing publisher key')
    return
  }

  if (data.mediaKey && !publisherKeys[data.mediaKey]) {
    publisherKeys[data.mediaKey] = data.publisherKey
  }

  chrome.braveRewards.getPublisherInfo(
    data.publisherKey, (result: RewardsExtension.Result, info?: RewardsExtension.Publisher) => {
      if (result === 0 && info) {
        getPublisherPanelInfo(tabId, data.publisherKey)
        return
      }

      // Failed to find publisher info corresponding to this key, so save it now
      if (result === 9) {
        savePublisherInfo(
          tabId,
          mediaType,
          data.url,
          data.publisherKey,
          data.publisherName,
          data.favIconUrl || '')
        return
      }
    })
}

const processGreaselionMessage = (msg: any, sender: chrome.runtime.MessageSender) => {
  if (!msg || !sender || !sender.tab) {
    return
  }

  const windowId = sender.tab.windowId
  if (!windowId) {
    return
  }

  const tabId = sender.tab.id
  if (!tabId) {
    return
  }

  const response = msg as GreaselionResponse
  if (!response.data) {
    console.error(`Received empty Greaselion response payload for ${msg.type} message`)
    return
  }

  switch (msg.type) {
    case 'GreaselionError': {
      const data = response.data as GreaselionErrorResponse
      handleGreaselionErrorResponse(tabId, response.mediaType, data)
      break
    }
    case 'SavePublisherVisit': {
      const data = response.data as SavePublisherVisitResponse
      handleSavePublisherVisitResponse(tabId, response.mediaType, data)
      break
    }
  }
}

chrome.runtime.onConnectExternal.addListener((port: chrome.runtime.Port) => {
  if (!port || !port.sender || !port.sender.id || !port.sender.tab || port.name !== 'Greaselion') {
    return
  }

  const tabId = port.sender.tab.id
  if (!tabId) {
    return
  }

  if (!greaselionPorts.get(tabId)) {
    greaselionPorts.set(tabId, port)
  }

  port.onMessage.addListener((msg: any, port: chrome.runtime.Port) => {
    if (!port.sender || !port.sender.id || !msg) {
      return
    }

    chrome.greaselion.isGreaselionExtension(port.sender.id, (valid: boolean) => {
      if (!valid) {
        return
      }
      switch (msg.type) {
        case 'MediaDurationHandlerRegistrationRequest': {
          const data = msg.data as MediaDurationHandlerRegistrationRequest
          handleMediaDurationHandlerRegistrationRequest(tabId, msg.mediaType, data)
          break
        }
        case 'MediaDurationMetadataResponse': {
          const data = msg.data as MediaDurationMetadataResponse
          handleMediaDurationMetadataResponse(tabId, msg.mediaType, data)
          break
        }
      }
    })
  })

  port.onDisconnect.addListener((port: chrome.runtime.Port) => {
    if (chrome.runtime.lastError) {
      console.error(`Greaselion port disconnected due to error: ${chrome.runtime.lastError}`)
    }
    if (port.sender && port.sender.tab && port.sender.tab.id) {
      greaselionPorts.delete(port.sender.tab.id)
    }
  })
})

chrome.runtime.onMessageExternal.addListener((msg: any, sender: chrome.runtime.MessageSender) => {
  if (!sender || !sender.id) {
    return
  }

  chrome.greaselion.isGreaselionExtension(sender.id, (valid: boolean) => {
    if (valid) {
      processGreaselionMessage(msg, sender)
    }
  })
})
