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
  tooltipPlacement: 'top' | 'top-start'
  centerUnderLogo?: boolean
}

export function NewTabTakeoverDisclosure(props: Props) {
  const actions = useBackgroundActions()
  const { shouldAutoDisplayNewTabTakeoverDisclosure, wallpaperId } =
    props.background

  const [tooltipVisible, setTooltipVisible] = React.useState(false)
  const tooltipShowTimer = React.useRef<number | undefined>(undefined)
  const autoDisplayTimer = React.useRef<number | undefined>(undefined)
  const autoDisplayHandledFor = React.useRef<string | null>(null)

  React.useEffect(() => {
    return () => {
      window.clearTimeout(tooltipShowTimer.current)
      window.clearTimeout(autoDisplayTimer.current)
    }
  }, [])

  // Auto-display is driven by its own timer, not `tooltipShowTimer`, since
  // `onMouseLeave` clears that timer and a pointer merely passing over the
  // label during the delay must not cancel the separately-tracked
  // auto-display timer.
  React.useEffect(() => {
    if (!shouldAutoDisplayNewTabTakeoverDisclosure) {
      return
    }
    if (autoDisplayHandledFor.current === wallpaperId) {
      return
    }
    autoDisplayHandledFor.current = wallpaperId
    let fired = false
    autoDisplayTimer.current = window.setTimeout(() => {
      fired = true
      setTooltipVisible(true)
      actions.notifyNewTabTakeoverDisclosureTooltipAutoDisplayed()
    }, tooltipShowDelay)
    return () => {
      window.clearTimeout(autoDisplayTimer.current)
      if (!fired) {
        // Timer never ran — don't leave this wallpaper marked "handled",
        // or a later return to it would silently skip auto-display.
        autoDisplayHandledFor.current = null
      }
    }
  }, [shouldAutoDisplayNewTabTakeoverDisclosure, wallpaperId, actions])

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

  return (
    <div data-css-scope={style.scope}>
      <Tooltip
        mode='default'
        placement={props.tooltipPlacement}
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
          className={
            props.centerUnderLogo
              ? 'new-tab-takeover-disclosure center-under-logo'
              : 'new-tab-takeover-disclosure'
          }
          data-theme='dark'
          onMouseEnter={scheduleShowTooltip}
          onMouseLeave={cancelScheduledShow}
          onClick={() => setTooltipVisible(true)}
        >
          {getString(S.NEW_TAB_TAKEOVER_DISCLOSURE_LABEL)}
        </span>
        <div
          slot='content'
          className='new-tab-takeover-disclosure-tooltip'
        >
          {formatString(getString(S.NEW_TAB_TAKEOVER_DISCLOSURE_TOOLTIP_TEXT), {
            $1: (content) => (
              <Link
                url={newTabTakeoverDisclosureLearnMoreURL}
                openInNewTab
                onClick={() =>
                  actions.notifyNewTabTakeoverDisclosureLearnMoreClicked()
                }
              >
                {content}
              </Link>
            ),
          })}
        </div>
      </Tooltip>
    </div>
  )
}
