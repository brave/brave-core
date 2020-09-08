/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Util
import rewardsPanelActions from './actions/rewardsPanelActions'

interface GreaselionError {
  errorMessage: string
}

interface MediaDurationMetadata {
  mediaKey: string
  duration: number
}

interface RegisterOnCompletedWebRequest {
  urlPattern: string
}

interface SavePublisherVisit {
  url: string
  publisherKey: string
  publisherName: string
  mediaKey?: string
  favIconUrl?: string
}

interface OnCompletedWebRequestRegistration {
  registered: boolean
}

// Maps webRequest.OnCompleted registrations by tab id
const onCompletedWebRequestRegistrationsByTabId = new Map<number, OnCompletedWebRequestRegistration>()

// Maps Greaselion ports by tab id
const greaselionPortsByTabId = new Map<number, chrome.runtime.Port>()

// Maps publisher keys by media key
const publisherKeysByMediaKey = new Map<string, string>()

const handleGreaselionError = (tabId: number, mediaType: string, data: GreaselionError) => {
  console.error(`Greaselion error: ${data.errorMessage}`)
}

const handleMediaDurationMetadata = (tabId: number, mediaType: string, data: MediaDurationMetadata) => {
  const publisherKey = publisherKeysByMediaKey.get(data.mediaKey)
  if (!publisherKey) {
    console.error(`Failed to handle media duration metadata: missing publisher key for media key ${data.mediaKey}`)
    return
  }

  chrome.braveRewards.updateMediaDuration(tabId, publisherKey, data.duration)
}

const handleRegisterOnCompletedWebRequest = (tabId: number, mediaType: string, data: RegisterOnCompletedWebRequest) => {
  // If we already registered a handler for this tab, exit early
  const handler = onCompletedWebRequestRegistrationsByTabId.get(tabId)
  if (handler && handler.registered) {
    return
  }

  // Mark this tab as registered
  onCompletedWebRequestRegistrationsByTabId.set(tabId, { registered: true })

  chrome.webRequest.onCompleted.addListener(
    // Listener
    function (details) {
      const port = greaselionPortsByTabId.get(tabId)
      if (!port) {
        return
      }
      port.postMessage({
        type: 'MediaDurationMetadata',
        mediaType,
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
        data.urlPattern
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

const handleSavePublisherVisit = (tabId: number, mediaType: string, data: SavePublisherVisit) => {
  if (!data.publisherKey) {
    console.error('Failed to handle publisher visit: missing publisher key')
    return
  }

  if (data.mediaKey && !publisherKeysByMediaKey.has(data.mediaKey)) {
    publisherKeysByMediaKey.set(data.mediaKey, data.publisherKey)
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

chrome.runtime.onConnectExternal.addListener((port: chrome.runtime.Port) => {
  if (!port || port.name !== 'Greaselion') {
    return
  }

  port.onMessage.addListener((msg: any, port: chrome.runtime.Port) => {
    if (!port.sender || !port.sender.id || !port.sender.tab || !msg) {
      return
    }

    const tabId = port.sender.tab.id
    if (!tabId) {
      return
    }

    if (!greaselionPortsByTabId.get(tabId)) {
      greaselionPortsByTabId.set(tabId, port)
    }

    chrome.greaselion.isGreaselionExtension(port.sender.id, (valid: boolean) => {
      if (!valid) {
        return
      }
      switch (msg.type) {
        case 'GreaselionError': {
          const data = msg.data as GreaselionError
          handleGreaselionError(tabId, msg.mediaType, data)
          break
        }
        case 'MediaDurationMetadata': {
          const data = msg.data as MediaDurationMetadata
          handleMediaDurationMetadata(tabId, msg.mediaType, data)
          break
        }
        case 'RegisterOnCompletedWebRequest': {
          const data = msg.data as RegisterOnCompletedWebRequest
          handleRegisterOnCompletedWebRequest(tabId, msg.mediaType, data)
          break
        }
        case 'SavePublisherVisit': {
          const data = msg.data as SavePublisherVisit
          handleSavePublisherVisit(tabId, msg.mediaType, data)
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
      greaselionPortsByTabId.delete(port.sender.tab.id)
    }
  })
})
