/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CreatorSite } from './app_state'

export function getCreatorIconSrc(site: CreatorSite) {
  return 'chrome://favicon2/?size=64&pageUrl=' +
    encodeURIComponent(site.icon || site.url)
}

export function getCreatorPlatformIcon(site: CreatorSite) {
  switch (site.platform) {
    case 'github':
      return 'social-github'
    case 'reddit':
      return 'social-reddit'
    case 'twitch':
      return 'social-twitch'
    case 'twitter':
      return 'social-x'
    case 'vimeo':
      return 'social-vimeo'
    case 'youtube':
      return 'social-youtube'
    case '':
      return ''
  }
}
