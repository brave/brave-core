// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// YouTube specific script to get metadata for media.
const siteSpecificDetector = { // eslint-disable-line no-unused-vars
  getThumbnail: function () {
    let thumbnail = window.ytplayer?.bootstrapPlayerResponse?.videoDetails?.thumbnail?.thumbnails
    if (thumbnail && Array.isArray(thumbnail)) {
      // The last one is known to be one with the hight resolution
      thumbnail = thumbnail[thumbnail.length - 1]?.url
    }
    return (typeof(thumbnail) === 'string' && (thumbnail.startsWith('/') || thumbnail.startsWith('http://') || thumbnail.startsWith('https://'))) 
        ? thumbnail : null
  },
  getMediaTitle: function (node) {
    const title = window.ytplayer?.bootstrapPlayerResponse?.videoDetails?.title
    return typeof(title) === 'string' ? title : null
  },
  getMediaAuthor: function (node) {
    const author = window.ytplayer?.bootstrapPlayerResponse?.videoDetails?.author
    return typeof(author) === 'string' ? author : null
  },
  getMediaDurationInSeconds: function (node) {
    let duration = window.ytplayer?.bootstrapPlayerResponse?.videoDetails?.lengthSeconds
    if (typeof(duration) == 'string') {
      duration = Number.parseFloat(duration)
      return Number.isNaN(duration) ? null : duration
    }
    return typeof(duration) === 'number' ? duration : null
  }
}
