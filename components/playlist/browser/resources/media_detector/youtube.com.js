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
    return thumbnail
  },
  getMediaTitle: function (node) {
    return window.ytplayer?.bootstrapPlayerResponse?.videoDetails?.title
  },
  getMediaAuthor: function (node) {
    return window.ytplayer?.bootstrapPlayerResponse?.videoDetails?.author
  },
  getMediaDurationInSeconds: function (node) {
    return window.ytplayer?.bootstrapPlayerResponse?.videoDetails?.lengthSeconds
  }
}
