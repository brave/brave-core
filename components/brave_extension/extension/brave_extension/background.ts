/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import './youtubedown'

import * as bluebird from 'bluebird'
global.Promise = bluebird

function promisifier (method: any) {
  // return a function
  return function promisified (...args: any[]) {
    // which returns a promise
    return new Promise((resolve: any) => {
      args.push(resolve)
      method.apply(this, args)
    })
  }
}

function promisifyAll (obj: object, list: string[]) {
  list.forEach(api => bluebird.promisifyAll(obj[api], { promisifier }))
}

// let chrome extension api support Promise
promisifyAll(chrome, [
  'browserAction',
  'tabs',
  'windows'
])

promisifyAll(chrome.storage, [
  'local'
])

bluebird.promisifyAll(chrome.braveShields, { promisifier })

require('./background/api')
require('./background/events')
require('./background/store')
if (chrome.test) {
  chrome.test.sendMessage('brave-extension-enabled')
}

function createPlaylist (url: string) {
  const ytdItems: chrome.bravePlaylists.YTDMediaItem[] = window.youtubedown_urls(url)
  if (ytdItems.length === 0) {
    console.log(' ##### got nothing from youtubedown_urls #####')
    return
  }
  const videoMediaFiles: chrome.bravePlaylists.CreateParamsMediaItem[] = makeMediaItems(ytdItems[0])
  let audioMediaFiles: chrome.bravePlaylists.CreateParamsMediaItem[] = []

  if (ytdItems.length > 1) {
    // Video is only available as separate video and audio files.
    // First item is always the video. Second item is always the audio.
    audioMediaFiles = makeMediaItems(ytdItems[1])
  }

  const thumbnailUrl = audioMediaFiles[0].thumb || videoMediaFiles[0].thumb
  const playlistName = getCleanPlaylistName(audioMediaFiles[0].title) || getCleanPlaylistName(videoMediaFiles[0].title)

  chrome.bravePlaylists.createPlaylist({
    thumbnailUrl,
    playlistName,
    videoMediaFiles,
    audioMediaFiles
  })
}

function getCleanPlaylistName (fileName: string) {
  return fileName && fileName.replace(/\[.*$/,'')
}

function makeMediaItems (item: chrome.bravePlaylists.YTDMediaItem) {
  let mediaFiles: chrome.bravePlaylists.CreateParamsMediaItem[]
  if (typeof item.url === 'string') {
    // Video is all in one file, but we'll pretend it's an array of 1 segment
    mediaFiles = [{ url: item.url, title: item.file, thumb: item.thumb }]
  } else {
    // Video is split into segments, which we will concatenate later
    mediaFiles = item.url.map(segment => ({ url: segment, title: item.file, thumb: item.thumb }))
  }
  return mediaFiles
}

chrome.bravePlaylists.isInitialized((init) => {
  if (init) {
    return
  }
  chrome.bravePlaylists.onInitialized.addListener(() => {
    chrome.bravePlaylists.onDownloadRequested.addListener((url) => {
      createPlaylist(url)
    })
  })
})
// Initialize playlist API
chrome.bravePlaylists.initialize()
