/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Tooltip from '@brave/leo/react/tooltip'

import { formatString } from '$web-common/formatString'
import { getString } from '../../lib/strings'
import { Link } from '../common/link'
import {
  SponsoredImageBackground,
  newTabTakeoverDisclosureLearnMoreURL,
} from '../../state/background_store'
import { useBackgroundActions } from '../../context/background_context'

import { style } from './new_tab_takeover_disclosure.style'

// Avoids showing the tooltip for a pointer that's just passing over the label.
const tooltipShowDelay = 500

// Gives the pointer time to reach "Learn more" without the tooltip closing.
const tooltipHideDelay = 500

interface Props {
  background: SponsoredImageBackground
  placement: 'top' | 'bottom'
}

export function NewTabTakeoverDisclosure(props: Props) {
  const actions = useBackgroundActions()
  const { shouldAutoShowNewTabTakeoverDisclosure, wallpaperId } =
    props.background

  const [tooltipVisible, setTooltipVisible] = React.useState(false)
  const tooltipShowTimer = React.useRef<number | undefined>(undefined)
  const autoShowTimer = React.useRef<number | undefined>(undefined)
  const autoShowHandledFor = React.useRef<string | null>(null)

  React.useEffect(() => {
    return () => {
      window.clearTimeout(tooltipShowTimer.current)
      window.clearTimeout(autoShowTimer.current)
    }
  }, [])

  // Auto-show is driven by its own timer, not `tooltipShowTimer`, since
  // `onMouseLeave` clears that timer and a pointer merely passing over the
  // label during the delay must not cancel a display the browser has already
  // counted against the auto-show cap.
  React.useEffect(() => {
    if (!shouldAutoShowNewTabTakeoverDisclosure) {
      return
    }
    if (autoShowHandledFor.current === wallpaperId) {
      return
    }
    autoShowHandledFor.current = wallpaperId
    autoShowTimer.current = window.setTimeout(
      () => setTooltipVisible(true),
      tooltipShowDelay,
    )
  }, [shouldAutoShowNewTabTakeoverDisclosure, wallpaperId])

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

  return (
    <div data-css-scope={style.scope}>
      <Tooltip
        mode='mini'
        placement={props.placement}
        positionStrategy='fixed'
        mouseleaveTimeout={tooltipHideDelay}
        visible={tooltipVisible}
        onVisibilityChange={({ visible }) => {
          if (!visible) {
            setTooltipVisible(false)
          }
        }}
      >
        <span
          className='new-tab-takeover-disclosure'
          tabIndex={0}
          onMouseEnter={scheduleShowTooltip}
          onMouseLeave={cancelScheduledShow}
          onFocus={() => setTooltipVisible(true)}
          onClick={() => setTooltipVisible(true)}
          onBlur={handleBlur}
        >
          {getString(S.NEW_TAB_TAKEOVER_DISCLOSURE_LABEL)}
        </span>
        <div
          slot='content'
          className='new-tab-takeover-disclosure-tooltip'
        >
          {formatString(
            getString(S.NEW_TAB_TAKEOVER_DISCLOSURE_TOOLTIP_TEXT),
            {
              $1: (content) => (
                <Link
                  url={newTabTakeoverDisclosureLearnMoreURL}
                  openInNewTab
                  onClick={() =>
                    actions.notifyNewTabTakeoverDisclosureLearnMoreClicked()
                  }
                  onBlur={handleBlur}
                >
                  {content}
                </Link>
              ),
            },
          )}
        </div>
      </Tooltip>
    </div>
  )
}
