// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as Background from '../../../../../common/Background'
import * as Feed from './feed'
import * as Publishers from './publishers'
import * as PublisherUserPrefs from './publisher-user-prefs'
import { getUnpaddedAsDataUrl } from './privateCDN'

// Setup event handlers
chrome.braveToday.onClearHistory.addListener(() => {
  // Parsed and weighted feed items have somewhat history-related
  // data since they have a `score` property which has a different
  // value if the user has visited the article's URL host recently.
  // So, clear the generated scores (and the entire generated feed)
  // when the user clears history.
  Feed.clearCache()
  console.debug('Cleared Brave Today feed from cache due to clear history event.')
})

// Setup listeners for messages from WebUI

import MessageTypes = Background.MessageTypes.Today
import Messages = BraveToday.Messages

Background.setListener<void>(
  MessageTypes.indicatingOpen,
  async function (payload, sender) {
    console.log('indicatingOpen')
    // TODO: only update if time has expired
    Feed.update()
    Publishers.update()
  }
)

Background.setListener<Messages.GetFeedResponse>(
  MessageTypes.getFeed,
  async function (req, sender, sendResponse) {
    // TODO: handle error
    const feed = await Feed.getOrFetchData()
    // Only wait once. If there was an error or no data then return nothing.
    // TODO: return error status
    console.log('sending', feed)
    sendResponse({
      feed
    })
  }
)

Background.setListener<Messages.GetPublishersResponse>(
  MessageTypes.getPublishers,
  async function (req, sender, sendResponse) {
    // TODO: handle error
    const publishers = await Publishers.getOrFetchData()
    // TODO(petemill): handle error
    sendResponse({ publishers })
  }
)

Background.setListener<Messages.GetImageDataResponse, Messages.GetImageDataPayload>(
  MessageTypes.getImageData,
  async function (req, sender, sendResponse) {
    // TODO: handle error
    const blob = await fetch(req.url).then(r => r.blob());
    // @ts-ignore (Blob.arrayBuffer does exist)
    const buffer = await blob.arrayBuffer()
    const dataUrl = await getUnpaddedAsDataUrl(buffer, 'image/jpg')
    sendResponse({
      dataUrl
    })
  }
)

Background.setListener<Messages.SetPublisherPrefResponse, Messages.SetPublisherPrefPayload>(
  MessageTypes.setPublisherPref,
  async function (req, sender, sendResponse) {
    const publisherId = req.publisherId
    const enabled: boolean | null = req.enabled
    await PublisherUserPrefs.setPublisherPref(publisherId, enabled)
    const publishers = await Publishers.update(true)
    sendResponse({ publishers })
  }
)

Background.setListener<Messages.IsFeedUpdateAvailableResponse, Messages.IsFeedUpdateAvailablePayload>(
  MessageTypes.isFeedUpdateAvailable,
  async function (req, sender, sendResponse) {
    const requestHash = req.hash
    const feed = await Feed.getOrFetchData()
    const isUpdateAvailable = !!(feed && feed.hash !== requestHash)
    sendResponse({ isUpdateAvailable })
  }
)

Background.setListener<Messages.ClearPrefsResponse, Messages.ClearPrefsPayload>(
  MessageTypes.resetPrefsToDefault,
  async function (req, sender, sendResponse) {
    await PublisherUserPrefs.clearPrefs()
    const publishers = await Publishers.update(true)
    sendResponse({ publishers })
})

// TODO: schedule to update feed
