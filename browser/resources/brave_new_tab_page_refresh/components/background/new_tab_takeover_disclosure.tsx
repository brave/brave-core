/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Tooltip from '@brave/nala/react/tooltip'

import { formatString } from '$web-common/formatString'
import { getString } from '../../lib/strings'
import { Link } from '../common/link'
import { NewTabTakeoverBackground } from '../../state/background_store'
import { useBackgroundActions } from '../../context/background_context'
import { horizontalContentPadding } from '../app.style'

import { style } from './new_tab_takeover_disclosure.style'

const learnMoreURL =
  'https://support.brave.app/hc/en-us/articles/35182999599501'

// Avoids showing the tooltip for a pointer that's just passing over the label.
const tooltipShowDelay = 500

// Gives the pointer time to reach "Learn more" without the tooltip closing.
const tooltipHideDelay = 500

interface Props {
  background: NewTabTakeoverBackground
}

export function NewTabTakeoverDisclosure(props: Props) {
  const actions = useBackgroundActions()
  const { shouldAutoDisplayNewTabTakeoverDisclosure } = props.background

  const [tooltipVisible, setTooltipVisible] = React.useState(false)
  const tooltipShowTimer = React.useRef<number | undefined>(undefined)
  const autoDisplayTimer = React.useRef<number | undefined>(undefined)
  const tooltipContentId = React.useId()
  const tooltipContainerRef = React.useRef<HTMLDivElement>(null)
  const hasEngagedWithDisclosure = React.useRef(false)

  React.useEffect(() => {
    return () => window.clearTimeout(tooltipShowTimer.current)
  }, [])

  React.useEffect(() => {
    if (!tooltipVisible) {
      hasEngagedWithDisclosure.current = false
    }
  }, [tooltipVisible])

  const recordEngagementWithDisclosure = React.useCallback(() => {
    hasEngagedWithDisclosure.current = true
  }, [])

  const closeTooltip = React.useCallback(() => {
    window.clearTimeout(tooltipShowTimer.current)
    window.clearTimeout(autoDisplayTimer.current)
    if (hasEngagedWithDisclosure.current) {
      actions.suppressNewTabTakeoverDisclosureAutoDisplay()
    }
    setTooltipVisible(false)
  }, [actions])

  React.useEffect(() => {
    if (!tooltipVisible) {
      return
    }
    function onKeyDown(event: KeyboardEvent) {
      if (event.key === 'Escape') {
        recordEngagementWithDisclosure()
        closeTooltip()
      }
    }
    function onPointerDown(event: PointerEvent) {
      if (!tooltipContainerRef.current?.contains(event.target as Node)) {
        closeTooltip()
      }
    }
    document.addEventListener('keydown', onKeyDown)
    document.addEventListener('pointerdown', onPointerDown)
    window.addEventListener('blur', closeTooltip)
    return () => {
      document.removeEventListener('keydown', onKeyDown)
      document.removeEventListener('pointerdown', onPointerDown)
      window.removeEventListener('blur', closeTooltip)
    }
  }, [tooltipVisible, recordEngagementWithDisclosure, closeTooltip])

  React.useEffect(() => {
    if (!shouldAutoDisplayNewTabTakeoverDisclosure) {
      return
    }
    function scheduleAutoDisplay() {
      autoDisplayTimer.current = window.setTimeout(() => {
        setTooltipVisible(true)
        actions.notifyNewTabTakeoverDisclosureAutoDisplayed()
      }, tooltipShowDelay)
    }
    function onVisibilityChange() {
      if (document.visibilityState === 'visible') {
        document.removeEventListener('visibilitychange', onVisibilityChange)
        scheduleAutoDisplay()
      }
    }
    if (document.visibilityState === 'visible') {
      scheduleAutoDisplay()
    } else {
      document.addEventListener('visibilitychange', onVisibilityChange)
    }
    return () => {
      document.removeEventListener('visibilitychange', onVisibilityChange)
      window.clearTimeout(autoDisplayTimer.current)
    }
  }, [shouldAutoDisplayNewTabTakeoverDisclosure, actions])

  function showTooltipAndRecordEngagement() {
    recordEngagementWithDisclosure()
    setTooltipVisible(true)
  }

  function scheduleShowTooltipOnMouseEnter() {
    window.clearTimeout(tooltipShowTimer.current)
    tooltipShowTimer.current = window.setTimeout(
      showTooltipAndRecordEngagement,
      tooltipShowDelay,
    )
  }

  function cancelScheduledShow() {
    window.clearTimeout(tooltipShowTimer.current)
  }

  return (
    <div
      ref={tooltipContainerRef}
      data-css-scope={style.scope}
      onMouseOver={() => {
        if (tooltipVisible) {
          recordEngagementWithDisclosure()
        }
      }}
    >
      <Tooltip
        mode='default'
        placement='top'
        shift={horizontalContentPadding}
        positionStrategy='fixed'
        mouseleaveTimeout={tooltipHideDelay}
        visible={tooltipVisible}
        onVisibilityChange={({ visible }) => {
          if (!visible) {
            closeTooltip()
          }
        }}
      >
        <span
          className='new-tab-takeover-disclosure'
          data-theme='dark'
          tabIndex={0}
          aria-describedby={tooltipContentId}
          onMouseEnter={scheduleShowTooltipOnMouseEnter}
          onMouseLeave={cancelScheduledShow}
          onClick={showTooltipAndRecordEngagement}
          onFocus={(event) => {
            if (event.currentTarget.matches(':focus-visible')) {
              showTooltipAndRecordEngagement()
            }
          }}
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
                url={learnMoreURL}
                onFocus={(event) => {
                  if (event.currentTarget.matches(':focus-visible')) {
                    showTooltipAndRecordEngagement()
                  }
                }}
                onBlur={closeTooltip}
                onClick={() =>
                  actions.suppressNewTabTakeoverDisclosureAutoDisplay()
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
