// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// This script is modified version of
// https://github.com/brave/brave-ios/blob/development/Client/Frontend/UserContent/UserScripts/Playlist.js
(function () {
  function fixUpRelativeUrl (url) {
    if (!url || typeof url !== 'string') {
      return url
    }

    if (!url.startsWith('http://') || !url.startsWith('https://')) {
      // Fix up relative path to absolute path
      url = new URL(url, window.location.origin).href
    }

    return url
  }

  function getNodeData (node) {
    let src = fixUpRelativeUrl(node.src)
    let mimeType = node.type
    let name = getMediaTitle(node)
    if (mimeType == null || typeof mimeType === 'undefined' || mimeType === '') {
      if (node.constructor.name === 'HTMLVideoElement') {
        mimeType = 'video'
      }

      if (node.constructor.name === 'HTMLAudioElement') {
        mimeType = 'audio'
      }

      if (node.constructor.name === 'HTMLSourceElement') {
        if (node.closest('video')) {
          mimeType = 'video'
        } else {
          mimeType = 'audio'
        }
      }
    }

    if (src && src !== '') {
      return [{
        'name': name,
        'src': src,
        'pageSrc': window.location.href,
        'pageTitle': document.title,
        'mimeType': mimeType,
        'duration': getMediaDurationInSeconds(node),
        'detected': true
      }]
    } else {
      let target = node
      let sources = []
      document.querySelectorAll('source').forEach(function (node) {
        if (node.src !== '') {
          if (node.closest('video') === target) {
            sources.push({
              'name': name,
              'src': fixUpRelativeUrl(node.src),
              'pageSrc': window.location.href,
              'pageTitle': document.title,
              'mimeType': mimeType,
              'duration': getMediaDurationInSeconds(target),
              'detected': true
            })
          }

          if (node.closest('audio') === target) {
            sources.push({
              'name': name,
              'src': fixUpRelativeUrl(node.src),
              'pageSrc': window.location.href,
              'pageTitle': document.title,
              'mimeType': mimeType,
              'duration': getMediaDurationInSeconds(target),
              'detected': true
            })
          }
        }
      })
      return sources
    }
  }

  function getAllVideoElements () {
    return document.querySelectorAll('video')
  }

  function getAllAudioElements () {
    return document.querySelectorAll('audio')
  }

  function getThumbnail () {
    const isThumbnailValid = (thumbnail) => { return thumbnail && thumbnail !== '' }

    let thumbnail = document.querySelector('meta[property="og:image"]')?.content
    return fixUpRelativeUrl(thumbnail)
  }

  function getMediaTitle (node) {
    const isTitleValid = (title) => { return title && title !== '' }

    let title = node.title
    if (!isTitleValid(title)) { title = document.title }

    return title
  }

  function getMediaAuthor (node) {
    // TODO(sko) Get metadata of author
    return null
  }

  function getMediaDurationInSeconds (node) {
    let duration = node.duration
    const isNan = (value) => { return typeof value === 'number' && Number.isNaN(value) }
    const isInfinite = (value) => { return typeof value === 'number' && (value === Infinity || value === -Infinity) }
    const clampDuration = (value) => {
      if (isNan(value)) {
        return 0.0
      }

      if (isInfinite(value)) {
        return Number.MAX_VALUE
      }
      return value
    }

    return clampDuration(duration)
  }

  let videoElements = getAllVideoElements() ?? []
  let audioElements = getAllAudioElements() ?? []
  // TODO(sko) These data could be incorrect when there're multiple items.
  // For now we're assuming that the first media is a representative one.
  const thumbnail = getThumbnail()
  const author = getMediaAuthor()

  let medias = []
  videoElements.forEach(e => medias = medias.concat(getNodeData(e)))
  audioElements.forEach(e => medias = medias.concat(getNodeData(e)))

  if (medias.length) {
    medias[0].thumbnail = thumbnail
    medias[0].author = author
  }

  return medias
})()
