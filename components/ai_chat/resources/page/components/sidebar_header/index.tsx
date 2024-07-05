/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import getPageHandlerInstance from '../../api/page_handler'
import DataContext from '../../state/context'
import FeatureButtonMenu from '../feature_button_menu'
import styles from './style.module.scss'

interface SidebarHeaderProps {
  onSidebarToggle?: () => void
}

export default function SidebarHeader(props: SidebarHeaderProps) {
  const context = React.useContext(DataContext)
  const { hasAcceptedAgreement } = context

  const handleEraseClick = () => {
    getPageHandlerInstance().pageHandler.clearConversationHistory()
  }

  const shouldDisplayEraseAction = context.conversationHistory.length >= 1

  return (
    <div className={styles.header}>
      <div className={styles.logoBody}>
        <div className={styles.divider} />
        <div className={styles.logo}>
          <Icon name='product-brave-leo' />
          <div className={styles.logoTitle}>Leo AI</div>
          {context.isPremiumUser && (
            <div className={styles.badgePremium}>PREMIUM</div>
          )}
        </div>
      </div>
      <div className={styles.actions}>
        {hasAcceptedAgreement && (
          <>
            {shouldDisplayEraseAction && (
              <Button
                fab
                kind='plain-faint'
                aria-label='Erase conversation history'
                title='Erase conversation history'
                onClick={handleEraseClick}
              >
                <Icon name='erase' />
              </Button>
            )}
            <FeatureButtonMenu />
          </>
        )}
      </div>
    </div>
  )
}
