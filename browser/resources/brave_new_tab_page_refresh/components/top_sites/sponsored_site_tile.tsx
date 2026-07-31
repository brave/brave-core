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

// Avoids showing the tooltip for a pointer that's just passing over the tile.
const tooltipShowDelay = 500

// Gives the pointer time to reach "Learn more" without the tooltip closing.
const tooltipHideDelay = 500

interface Props {
  site: SponsoredSite
  onContextMenu: (event: React.MouseEvent) => void
}

export function SponsoredSitesTile(props: Props) {
  const { relativeImageUrl, title, adDisclosure, targetUrl } = props.site

  const [tooltipVisible, setTooltipVisible] = React.useState(false)
  const tooltipShowTimer = React.useRef<number | undefined>(undefined)

  React.useEffect(() => {
    return () => window.clearTimeout(tooltipShowTimer.current)
  }, [])

  function scheduleShowTooltip() {
    window.clearTimeout(tooltipShowTimer.current)
    tooltipShowTimer.current = window.setTimeout(
      () => setTooltipVisible(true),
      tooltipShowDelay,
    )
  }

  function cancelScheduledShow() {
    window.clearTimeout(tooltipShowTimer.current)
  }

  // Keeps the tooltip open when focus moves to its "Learn more" link, since
  // that link sits outside Leo's trigger element and would otherwise be
  // treated as a blur.
  function handleBlur(e: React.FocusEvent) {
    const host = e.currentTarget.closest('leo-tooltip')
    if (host && host.contains(e.relatedTarget as Node | null)) {
      setTooltipVisible(true)
    } else {
      setTooltipVisible(false)
    }
  }

  // Tooltip wraps the whole tile rather than just the disclosure label below,
  // since scoping it to the label would nest the tooltip's "Learn more" link
  // inside the tile's own anchor, which is invalid HTML and breaks keyboard
  // access to the link.
  return (
    <Tooltip
      mode='mini'
      placement='bottom'
      positionStrategy='fixed'
      mouseleaveTimeout={tooltipHideDelay}
      visible={tooltipVisible}
      onVisibilityChange={({ visible }) => {
        if (!visible) {
          setTooltipVisible(false)
        }
      }}
    >
      <a
        className='top-site-tile'
        href={sanitizeExternalURL(targetUrl)}
        draggable={false}
        onContextMenu={props.onContextMenu}
      >
        <span className='top-site-icon'>
          <img src={sanitizeExternalURL(relativeImageUrl)} />
        </span>
        <span className='top-site-title'>{title}</span>
        {adDisclosure && (
          <span
            className='top-site-ad-disclosure'
            tabIndex={0}
            onMouseEnter={scheduleShowTooltip}
            onMouseLeave={cancelScheduledShow}
            onFocus={() => setTooltipVisible(true)}
            onBlur={handleBlur}
          >
            {adDisclosure}
          </span>
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
              onBlur={handleBlur}
            >
              {content}
            </Link>
          ),
        })}
      </div>
    </Tooltip>
  )
}
