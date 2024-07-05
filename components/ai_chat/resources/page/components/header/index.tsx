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

interface PageTitleHeaderProps {
  title?: string
}

export function PageTitleHeader(props: PageTitleHeaderProps) {
  const context = React.useContext(DataContext)
  const { hasAcceptedAgreement } = context

  const shouldDisplayEraseAction = context.conversationHistory.length >= 1

  const handleEraseClick = () => {
    getPageHandlerInstance().pageHandler.clearConversationHistory()
  }

  return (
    <div className={styles.header}>
      {props.title ? (
        <div className={styles.pageTitle}>
          <Button
            kind='plain-faint'
            fab
            onClick={() => {}}
          >
            <Icon name='arrow-left' />
          </Button>
          <div className={styles.pageText}>{props.title}</div>
        </div>
      ) : (
        <div className={styles.logo}>
          <Icon name='product-brave-leo' />
          <div className={styles.logoTitle}>Leo AI</div>
          {context.isPremiumUser && (
            <div className={styles.badgePremium}>PREMIUM</div>
          )}
        </div>
      )}
      <div className={styles.actions}>
        {hasAcceptedAgreement && (
          <>
            <Button
              fab
              kind='plain-faint'
              aria-label='Launch'
              title='Launch'
              onClick={() => {}}
            >
              <Icon name='launch' />
            </Button>
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
            <Button
              fab
              kind='plain-faint'
              aria-label='Close'
              title='Close'
              className={styles.closeButton}
              onClick={() => getPageHandlerInstance().pageHandler.closePanel()}
            >
              <Icon name='close' />
            </Button>
          </>
        )}
      </div>
    </div>
  )
}

export default function SidebarHeader() {
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
