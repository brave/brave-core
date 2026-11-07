// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  CrWebApi,
  gCrWeb,
} from '//ios/web/public/js_messaging/resources/gcrweb.js'

// The audible video that was playing while the scene resigned active, before
// WebKit pauses video for backgrounding. One per document so a second play()
// does not restart its buffer.
let playingVideo: HTMLVideoElement | null = null

function isAudibleAndPlaying(video: HTMLVideoElement): boolean {
  return !video.paused && !video.ended && !video.muted && video.volume > 0
}

function findPlayingVideo(root: ParentNode): HTMLVideoElement | null {
  for (const video of root.querySelectorAll<HTMLVideoElement>('video')) {
    if (isAudibleAndPlaying(video)) {
      return video
    }
  }
  for (const element of root.querySelectorAll('*')) {
    if (!element.shadowRoot) {
      continue
    }
    const found = findPlayingVideo(element.shadowRoot)
    if (found) {
      return found
    }
  }
  return null
}

function markPlayingVideos(): void {
  playingVideo = findPlayingVideo(document)
}

function resumePlayingVideos(): void {
  const video = playingVideo
  if (!video) {
    return
  }
  const resume = (): void => {
    if (playingVideo !== video || video.ended || !video.paused) {
      return
    }
    playingVideo = null
    video.play().catch(() => {})
  }
  // WebKit applies its background pause after the scene enters the background,
  // so the video may still be playing when this runs. Allow for a small delay before resuming.
  setTimeout(resume, 250)
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
