/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CreatorSite, CreatorBanner } from './app_store'
import { externalImageURL, faviconURL } from './external_url'

export function getCreatorIconSrc(
  site: CreatorSite,
  banner: CreatorBanner | null = null,
) {
  if (banner && banner.logo) {
    const src = externalImageURL(banner.logo)
    if (src) {
      return src
    }
  }
  return faviconURL(site.url)
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
