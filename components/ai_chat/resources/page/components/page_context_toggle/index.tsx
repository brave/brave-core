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
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'
import SiteTitles from '../site_titles'

function PageContextToggle() {
  const conversationContext = useConversation()
  const [isTooltipVisible, setIsTooltipVisible] = React.useState(false)

  const handleToggleChange = ({ checked }: { checked: boolean }) => {
    conversationContext.updateShouldSendPageContents(checked)
  }

  const toggleTooltipVisibility = () => {
    setIsTooltipVisible(state => !state)
  }

  return (
    <div className={styles.box}>
      <Toggle
        size='small'
        onChange={handleToggleChange}
        checked={conversationContext.shouldSendPageContents}
        className={styles.toggle}
      >
        <span slot="on-icon" />
        <div className={styles.label}>
          <span>{getLocale('contextToggleLabel')}</span>
          <Tooltip
            visible={isTooltipVisible}
            mode="default"
            className={styles.tooltip}
            onVisibilityChange={(detail) => {
              setTimeout(() => {
                setIsTooltipVisible(detail.visible)
              })
            }}
          >
            <div
              slot='content'
              className={styles.tooltipContent}
              onClick={(e) => {
                // inner content click/tap shouldn't change parent's toggle
                e.preventDefault()
              }}
            >
              <div className={styles.tooltipInfo}>
                {getLocale('contextToggleTooltipInfo')}
              </div>
              <div className={styles.tooltipSiteTitle}>
                <SiteTitles size='small' />
              </div>
            </div>
            <Button
              fab
              kind='plain-faint'
              className={styles.tooltipButton}
              onClick={toggleTooltipVisibility}
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
