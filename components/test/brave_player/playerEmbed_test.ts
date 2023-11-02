/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  EmbedURL,
  getFrameSrc
} from '../../brave_player/browser/resources/playerEmbed'

describe('playerEmbedTest', () => {
  describe('getFrameSrc', () => {
    it(`returns empty string if pathname doesn't have video id or isn't for youtube`, () => {
      expect(
        getFrameSrc(new URL('chrome-untrusted://player-embed/youtube').pathname)
      ).toBe('')
      expect(
        getFrameSrc(new URL('chrome-untrusted://player-embed').pathname)
      ).toBe('')
      expect(
        getFrameSrc(new URL('chrome-untrusted://player-embed/vimeo').pathname)
      ).toBe('')
    })

    it('returns YouTube embed URL with given video id', () => {
      const videoId = 'dQw4w9WgXcQ'
      expect(
        getFrameSrc(
          new URL(
            `chrome-untrusted://player/youtube/${encodeURIComponent(videoId)}`
          ).pathname
        )
      ).toBe(EmbedURL('youtube', videoId))
    })
  })
})
