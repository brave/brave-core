// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  CrWebApi,
  gCrWeb,
} from '//ios/web/public/js_messaging/resources/gcrweb.js'

// Set while the scene is resigning active, before WebKit pauses video for backgrounding.
const wasPlaying = new WeakMap<HTMLVideoElement, boolean>()

function visitVideos(
  root: ParentNode,
  visit: (video: HTMLVideoElement) => void,
): void {
  for (const video of root.querySelectorAll<HTMLVideoElement>('video')) {
    visit(video)
  }
  for (const element of root.querySelectorAll('*')) {
    if (element.shadowRoot) {
      visitVideos(element.shadowRoot, visit)
    }
  }
}

function markPlayingVideos(): void {
  visitVideos(document, (video) => {
    wasPlaying.set(video, !video.paused && !video.ended)
  })
}

function resumePlayingVideos(): void {
  const resume = (): void => {
    visitVideos(document, (video) => {
      if (video.ended || !wasPlaying.get(video) || !video.paused) {
        return
      }
      wasPlaying.set(video, false)
      video.play()
    })
  }
  resume()
  setTimeout(resume, 0)
  setTimeout(resume, 100)
}

const videoBackgroundPlaybackApi = new CrWebApi('videoBackgroundPlayback')
videoBackgroundPlaybackApi.addFunction('markPlayingVideos', markPlayingVideos)
videoBackgroundPlaybackApi.addFunction(
  'resumePlayingVideos',
  resumePlayingVideos,
)
if (!gCrWeb.hasRegisteredApi('videoBackgroundPlayback')) {
  gCrWeb.registerApi(videoBackgroundPlaybackApi)
}
