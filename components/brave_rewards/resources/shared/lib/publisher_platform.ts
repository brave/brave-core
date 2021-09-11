/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export type PublisherPlatform =
  'twitter' |
  'youtube' |
  'twitch' |
  'reddit' |
  'vimeo' |
  'github'

export function getPublisherPlatformName (platform: PublisherPlatform) {
  switch (platform) {
    case 'twitter': return 'Twitter'
    case 'youtube': return 'YouTube'
    case 'twitch': return 'Twitch'
    case 'reddit': return 'Reddit'
    case 'vimeo': return 'Vimeo'
    case 'github': return 'GitHub'
  }
}
