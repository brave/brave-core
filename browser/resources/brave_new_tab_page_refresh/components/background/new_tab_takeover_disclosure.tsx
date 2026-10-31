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
import { horizontalContentPadding } from '../app.style'

import { style } from './new_tab_takeover_disclosure.style'

// Avoids showing the tooltip for a pointer that's just passing over the label.
const tooltipShowDelay = 500

// Gives the pointer time to reach "Learn more" without the tooltip closing.
const tooltipHideDelay = 500

interface Props {
  background: SponsoredImageBackground
}

export function NewTabTakeoverDisclosure(props: Props) {
  const actions = useBackgroundActions()
  const { shouldAutoDisplayNewTabTakeoverDisclosure } = props.background

  const [tooltipVisible, setTooltipVisible] = React.useState(false)
  const tooltipShowTimer = React.useRef<number | undefined>(undefined)
  const autoDisplayTimer = React.useRef<number | undefined>(undefined)
  const tooltipContentId = React.useId()

  React.useEffect(() => {
    return () => window.clearTimeout(tooltipShowTimer.current)
  }, [])

  React.useEffect(() => {
    if (!tooltipVisible) {
      return
    }
    function onKeyDown(event: KeyboardEvent) {
      if (event.key === 'Escape') {
        window.clearTimeout(tooltipShowTimer.current)
        window.clearTimeout(autoDisplayTimer.current)
        setTooltipVisible(false)
      }
    }
    document.addEventListener('keydown', onKeyDown)
    return () => document.removeEventListener('keydown', onKeyDown)
  }, [tooltipVisible])

  React.useEffect(() => {
    if (!shouldAutoDisplayNewTabTakeoverDisclosure) {
      return
    }
    const timer = window.setTimeout(() => {
      setTooltipVisible(true)
      actions.notifyNewTabTakeoverDisclosureTooltipAutoDisplayed()
    }, tooltipShowDelay)
    autoDisplayTimer.current = timer
    return () => window.clearTimeout(timer)
  }, [shouldAutoDisplayNewTabTakeoverDisclosure, actions])

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
        placement='top'
        shift={horizontalContentPadding}
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
          data-theme='dark'
          tabIndex={0}
          aria-describedby={tooltipContentId}
          onMouseEnter={scheduleShowTooltip}
          onMouseLeave={cancelScheduledShow}
          onClick={() => setTooltipVisible(true)}
          onFocus={() => setTooltipVisible(true)}
          onBlur={() => setTooltipVisible(false)}
        >
          {getString(S.NEW_TAB_TAKEOVER_DISCLOSURE_LABEL)}
        </span>
        <div
          id={tooltipContentId}
          slot='content'
          className='new-tab-takeover-disclosure-tooltip'
        >
          {formatString(getString(S.NEW_TAB_TAKEOVER_DISCLOSURE_TOOLTIP_TEXT), {
            $1: (content) => (
              <Link
                url={newTabTakeoverDisclosureLearnMoreURL}
                openInNewTab
                onFocus={() => setTooltipVisible(true)}
                onBlur={() => setTooltipVisible(false)}
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
