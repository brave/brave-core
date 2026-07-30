// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { sendWebKitMessage } from '//ios/web/public/js_messaging/resources/utils.js'

// Identifies which video element a message is about, since a page can have
// more than one playing at once.
let nextPlayerId = 0
const playerIds = new WeakMap<HTMLVideoElement, number>()

function getPlayerId(video: HTMLVideoElement): number {
  let playerId = playerIds.get(video)
  if (playerId === undefined) {
    playerId = nextPlayerId++
    playerIds.set(video, playerId)
  }
  return playerId
}

function sendMessage(playerId: number, playing: boolean) {
  sendWebKitMessage('AdsMediaReportingMessageHandler', {
    'playerId': playerId,
    'isPlaying': playing,
  })
}

function isPlayingVideoWithAudio(video: HTMLVideoElement): boolean {
  return !video.paused && !video.muted
}

function hookVideoElement(video: HTMLVideoElement) {
  const playerId = getPlayerId(video)
  video.addEventListener('pause', () => sendMessage(playerId, false), false)
  video.addEventListener(
    'playing',
    () => sendMessage(playerId, isPlayingVideoWithAudio(video)),
    false,
  )
  video.addEventListener(
    'volumechange',
    () => sendMessage(playerId, isPlayingVideoWithAudio(video)),
    false,
  )
}

document.querySelectorAll('video').forEach(hookVideoElement)

const observer = new MutationObserver(function (mutations: MutationRecord[]) {
  mutations.forEach(function (mutation: MutationRecord) {
    mutation.addedNodes.forEach(function (node: Node) {
      if (node instanceof HTMLVideoElement) {
        hookVideoElement(node)
      } else if (node instanceof HTMLElement) {
        // Some sites inject a container element that already has video
        // descendants, so the video itself is never a direct added node.
        node.querySelectorAll('video').forEach(hookVideoElement)
      }
    })
  })
})
observer.observe(document, { subtree: true, childList: true })
