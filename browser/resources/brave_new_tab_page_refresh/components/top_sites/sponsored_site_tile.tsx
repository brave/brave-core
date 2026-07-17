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
import { Link } from '../common/link'

interface Props {
  site: SponsoredSite
  onContextMenu: (event: React.MouseEvent) => void
}

export function SponsoredSitesTile(props: Props) {
  const { relativeImageUrlSpec, title, adDisclosure, targetUrl } = props.site

  return (
    <Tooltip
      mode='default'
      placement='bottom'
      positionStrategy='fixed'
    >
      <a
        className='top-site-tile'
        href={sanitizeExternalURL(targetUrl.url)}
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
        className='sponsored-site-tooltip'
      >
        {formatString(getString(S.NEW_TAB_SPONSORED_SITE_TOOLTIP_TEXT), {
          $1: title,
          $2: (content) => (
            <Link
              url={sponsoredSiteLearnMoreURL}
              openInNewTab
            >
              {content}
            </Link>
          ),
        })}
      </div>
    </Tooltip>
  )
}
