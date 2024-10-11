/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import getAPI, { Conversation } from '../../api'
import FeatureButtonMenu, { Props as FeatureButtonMenuProps } from '../feature_button_menu'
import styles from './style.module.scss'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import { getLocale } from '$web-common/locale'

const getTitle = (activeConversation?: Conversation) => activeConversation?.title
  || activeConversation?.summary
  || getLocale('conversationListUntitled')

export const PageTitleHeader = React.forwardRef(function (props: FeatureButtonMenuProps, ref: React.Ref<HTMLDivElement>) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const shouldDisplayEraseAction = !aiChatContext.isStandalone &&
    conversationContext.conversationHistory.length >= 1

  const handleEraseClick = () => {
    aiChatContext.onNewConversation()
  }

  const activeConversation = aiChatContext.visibleConversations.find(c => c.uuid === conversationContext.conversationUuid)
  const showTitle = (!aiChatContext.isDefaultConversation || aiChatContext.isStandalone)

  return (
    <div className={styles.header} ref={ref}>
      {showTitle ? (
        <div className={styles.pageTitle}>
          <Button
            kind='plain-faint'
            fab
            onClick={() => { aiChatContext.onSelectConversationUuid(undefined) }}
          >
            <Icon name='arrow-left' />
          </Button>
          <div className={styles.pageText}>{getTitle(activeConversation)}</div>
        </div>
      ) : (
        <div className={styles.logo}>
          <Icon name='product-brave-leo' />
          <div className={styles.logoTitle}>Leo AI</div>
          {aiChatContext.isPremiumUser && (
            <div className={styles.badgePremium}>PREMIUM</div>
          )}
        </div>
      )}
      <div className={styles.actions}>
        {aiChatContext.hasAcceptedAgreement && (
          <>
            {/* <Button
                fab
                kind='plain-faint'
                aria-label='Launch'
                title='Launch'
                onClick={() => {}}
              >
                <Icon name='launch' />
              </Button> */}
            {shouldDisplayEraseAction && (
              <Button
                fab
                kind='plain-faint'
                aria-label='Erase conversation history'
                title='Erase conversation history'
                onClick={handleEraseClick}
              >
                <Icon name={aiChatContext.isHistoryEnabled ? 'plus-add' : 'erase'} />
              </Button>
            )}
            <FeatureButtonMenu {...props} />
            <Button
              fab
              kind='plain-faint'
              aria-label='Close'
              title='Close'
              className={styles.closeButton}
              onClick={() => getAPI().UIHandler.closeUI()}
            >
              <Icon name='close' />
            </Button>
          </>
        )}
      </div>
    </div>
  )
})

export function SidebarHeader(props: FeatureButtonMenuProps) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const handleEraseClick = () => {
    aiChatContext.onNewConversation()
  }

  const shouldDisplayEraseAction =
    conversationContext.conversationHistory.length >= 1

  return (
    <div className={styles.header}>
      <div className={styles.logoBody}>
        <div className={styles.divider} />
        <div className={styles.logo}>
          <Icon name='product-brave-leo' />
          <div className={styles.logoTitle}>Leo AI</div>
          {aiChatContext.isPremiumUser && (
            <div className={styles.badgePremium}>PREMIUM</div>
          )}
        </div>
      </div>
      <div className={styles.actions}>
        {aiChatContext.hasAcceptedAgreement && (
          <>
            {shouldDisplayEraseAction && (
              <Button
                fab
                kind='plain-faint'
                aria-label='Erase conversation history'
                title='Erase conversation history'
                onClick={handleEraseClick}
              >
                <Icon name='plus-add' />
              </Button>
            )}
            <FeatureButtonMenu {...props} />
          </>
        )}
      </div>
    </div>
  )
}
