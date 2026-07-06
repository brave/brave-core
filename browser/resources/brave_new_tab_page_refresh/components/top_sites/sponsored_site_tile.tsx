/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Tooltip from '@brave/leo/react/tooltip'

import {
  SponsoredSite,
  sponsoredSiteLearnMoreURL,
} from '../../state/top_sites_store'
import { sanitizeExternalURL } from '../../lib/url_sanitizer'
import { formatString } from '$web-common/formatString'
import { getString } from '../../lib/strings'

interface Props {
  site: SponsoredSite
  suppressTooltip: boolean
  onContextMenu: (event: React.MouseEvent) => void
}

export function SponsoredSitesTile(props: Props) {
  const { relativeImageUrlSpec, title, adDisclosure, targetUrl } = props.site

  return (
    <Tooltip
      mode='default'
      placement='bottom'
      positionStrategy='fixed'
      // Avoid overlapping the tile's own right-click context menu.
      visible={props.suppressTooltip ? false : undefined}
    >
      <a
        className='top-site-tile'
        href={sanitizeExternalURL(targetUrl.url)}
        // Sponsored sites are not reorderable; suppress the browser's
        // default anchor drag behavior too, not just the custom handler.
        draggable={false}
        onContextMenu={props.onContextMenu}
      >
        <span className='top-site-icon'>
          <img src={sanitizeExternalURL(relativeImageUrlSpec)} />
        </span>
        <span className='top-site-title'>{title}</span>
        {adDisclosure && (
          <span className='top-site-ad-disclosure'>{adDisclosure}</span>
        )}
      </a>
      <div
        slot='content'
        style={{ maxWidth: '320px' }}
      >
        {formatString(getString(S.NEW_TAB_SPONSORED_SITE_TOOLTIP_TEXT), {
          $1: title,
          $2: (content) => (
            <a
              href={sponsoredSiteLearnMoreURL}
              target='_blank'
              rel='noreferrer'
            >
              {content}
            </a>
          ),
        })}
      </div>
    </Tooltip>
  )
}
