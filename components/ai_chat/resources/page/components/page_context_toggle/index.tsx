/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Toggle from '@brave/leo/react/toggle'
import Tooltip from '@brave/leo/react/tooltip'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'

import styles from './style.module.scss'
import SiteTitle from '../site_title'
import DataContext from '../../state/context'

function PageContextToggle() {
  const [showTooltip, setShowTooltip] = React.useState(false)
  const context = React.useContext(DataContext)

  const handleToggleChange = ({ checked }: { checked: boolean }) => {
    context.updateShouldSendPageContents(checked)
  }

  const handleInfoIconTouchStart = (e: React.TouchEvent<HTMLDivElement>) => {
    e.preventDefault()
    setShowTooltip(true)
  }

  const handleInfoIconTouchEnd = (e: React.TouchEvent<HTMLDivElement>) => {
    e.preventDefault()
    setShowTooltip(false)
  }

  return (
    <div className={styles.box}>
      <Toggle
        size='small'
        onChange={handleToggleChange}
        checked={context.shouldSendPageContents}
        className={styles.toggle}
      >
        <span slot="on-icon" />
        <div className={styles.label}>
          <span>{getLocale('contextToggleLabel')}</span>
          <Tooltip visible={showTooltip}>
            <div
              slot='content'
              className={styles.tooltipContainer}
            >
              <div className={styles.tooltipInfo}>
                {getLocale('contextToggleTooltipInfo')}
              </div>
              <div className={styles.tooltipSiteTitle}>
                <SiteTitle size='small' />
              </div>
            </div>
            <div
              onMouseOver={() => setShowTooltip(true)}
              onMouseOut={() => setShowTooltip(false)}
              onTouchStart={handleInfoIconTouchStart}
              onTouchEnd={handleInfoIconTouchEnd}
              onFocus={() => setShowTooltip(true)}
              onBlur={() => setShowTooltip(false)}
            >
              <Icon name='info-outline' />
            </div>
          </Tooltip>
        </div>
      </Toggle>
    </div>
  )
}

export default PageContextToggle
