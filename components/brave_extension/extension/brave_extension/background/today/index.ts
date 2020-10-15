// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as Background from '../../../../../common/Background'
import * as Feed from './feed'
import * as Publishers from './publishers'
import { getUnpaddedAsDataUrl } from './privateCDN'

// TODO: make this a shared (common) thing and do explicit types for payloads like
// we do with redux action payloads.

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
    console.log('asked to get feed')
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
    console.log('asked to get publishers')
    const publishers = await Publishers.getOrFetchData()
    // TODO: handle error
    console.log('sending', publishers)
    sendResponse({ publishers })
  }
)

Background.setListener<Messages.GetImageDataResponse, Messages.GetImageDataPayload>(
  MessageTypes.getImageData,
  async function (req, sender, sendResponse) {
    console.log('asked for image')
    const blob = await fetch(req.url).then(r => r.blob());
    // @ts-ignore (Blob.arrayBuffer does exist)
    const buffer = await blob.arrayBuffer()
    const dataUrl = await getUnpaddedAsDataUrl(buffer, 'image/jpg')
    sendResponse({
      dataUrl
    })
  }
)

// TODO: schedule to update feed
