/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import styles from './style.module.scss'
import { SiteInfo } from '../../api/page_handler'

interface SiteTitleProps {
  siteInfo?: SiteInfo
  favIconUrl?: string
  onDisconnectButtonClick?: () => void
}

type Timer = ReturnType<typeof setTimeout>

const HOVER_TRANSITION_DELAY = 500

interface ToolTipProps {
  isVisible: boolean
}

function Tooltip(props: ToolTipProps) {
  const tooltipStyles = classnames({
    [styles.tooltip]: true,
    [styles.tooltipVisible]: props.isVisible
  })

  return (
    <div role="tooltip" id="page-content-warning-tooltip" className={tooltipStyles}>
      {getLocale('pageContentWarning')}
    </div>
  )
}

function SiteTitle (props: SiteTitleProps) {
  const [isTooltipVisible, setIsTooltipVisible] = React.useState(false)
  const timerId = React.useRef<Timer | undefined>();

  const showTooltipWithDelay = () => {
    timerId.current = setTimeout(() => {
      setIsTooltipVisible(true)
    }, HOVER_TRANSITION_DELAY)
  }

  const hideTooltip = () => {
    timerId.current && clearTimeout(timerId.current)
    setIsTooltipVisible(false)
  }

  const handleOnKeyDown = (e: React.KeyboardEvent<HTMLDivElement>) => {
    if (e.key === "Escape") {
      setIsTooltipVisible(false)
    }
  }

   return (
    <div className={styles.box}>
      <Tooltip isVisible={isTooltipVisible} />
      <div
        aria-describedby='page-content-warning-tooltip'
        tabIndex={0}
        className={styles.favIconBox}
        onMouseOver={showTooltipWithDelay}
        onMouseOut={hideTooltip}
        onTouchStart={() => setIsTooltipVisible(true)}
        onTouchEnd={() => setIsTooltipVisible(false)}
        onFocus={() => setIsTooltipVisible(true)}
        onBlur={() => setIsTooltipVisible(false)}
        onKeyDown={handleOnKeyDown}
      >
        { props.favIconUrl && <img src={props.favIconUrl} /> }
      </div>
      <div className={styles.titleBox}>
        <p className={styles.title}>{props.siteInfo?.title}</p>
      </div>
      <div className={styles.actions}>
        <Button kind="plain" onClick={props.onDisconnectButtonClick}>
          <Icon name="link-broken" />
        </Button>
      </div>
    </div>
  )
}

export default SiteTitle
