/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Toggle from '@brave/leo/react/toggle'
import Tooltip from '@brave/leo/react/tooltip'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'

import styles from './style.module.scss'
import SiteTitle from '../site_title'
import DataContext from '../../state/context'

function PageContextToggle() {
  const context = React.useContext(DataContext)

  const handleToggleChange = ({ checked }: { checked: boolean }) => {
    context.updateShouldSendPageContents(checked)
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
          <Tooltip
            mode="default"
            className={styles.tooltip}
          >
            <div
              slot='content'
              className={styles.tooltipContent}
            >
              <div className={styles.tooltipInfo}>
                {getLocale('contextToggleTooltipInfo')}
              </div>
              <div className={styles.tooltipSiteTitle}>
                <SiteTitle size='small' />
              </div>
            </div>
            <Button
              kind='plain-faint'
              className={styles.tooltipButton}
            >
              <Icon name='info-outline' />
            </Button>
          </Tooltip>
        </div>
      </Toggle>
    </div>
  )
}

export default PageContextToggle
