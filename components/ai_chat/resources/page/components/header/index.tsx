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
import { useSelectedConversation } from '../../routes'

const Logo = ({ isPremium }: { isPremium: boolean }) => <div className={styles.logo}>
  <Icon name='product-brave-leo' />
  <div className={styles.logoTitle}>Leo AI</div>
  {isPremium && (
    <div className={styles.badgePremium}>PREMIUM</div>
  )}
</div>

const getTitle = (activeConversation?: Conversation) => activeConversation?.title
  || activeConversation?.summary
  || getLocale('conversationListUntitled')


const newChatButtonLabel = getLocale('newChatButtonLabel')

export const PageTitleHeader = React.forwardRef(function (props: FeatureButtonMenuProps, ref: React.Ref<HTMLDivElement>) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const shouldDisplayEraseAction = !aiChatContext.isStandalone &&
    conversationContext.conversationHistory.length >= 1

  const activeConversation = aiChatContext.visibleConversations.find(c => c.uuid === conversationContext.conversationUuid)
  const conversationId = useSelectedConversation()
  const isDefault = conversationId === 'default'
  const showTitle = aiChatContext.isStandalone || !isDefault

  return (
    <div className={styles.header} ref={ref}>
      {showTitle ? (
        <div className={styles.pageTitle}>
          {!isDefault && !aiChatContext.isStandalone && <Button
            kind='plain-faint'
            fab
            onClick={() => location.href = "/default"}
          >
            <Icon name='arrow-left' />
          </Button>}
          <div className={styles.pageText}>{getTitle(activeConversation)}</div>
        </div>
      )
        : <Logo isPremium={aiChatContext.isPremiumUser} />}
      <div className={styles.actions}>
        {aiChatContext.hasAcceptedAgreement && (
          <>
            {shouldDisplayEraseAction && (
              <Button
                fab
                kind='plain-faint'
                aria-label={newChatButtonLabel}
                title={newChatButtonLabel}
                onClick={() => location.href = "/"}
              >
                <Icon name={aiChatContext.isHistoryEnabled ? 'edit-box' : 'erase'} />
              </Button>
            )}
            {!aiChatContext.isStandalone &&
              <Button
                fab
                kind='plain-faint'
                aria-label="Open full page"
                title="Open full page"
                onClick={() => {
                  // TODO(fallaciousreasoning): Wire this up to a new mojom interface.
                }}
              >
                <Icon name='expand' />
              </Button>}
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

export function SidebarHeader() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const canStartNewConversation = conversationContext.conversationHistory.length >= 1
    && aiChatContext.hasAcceptedAgreement

  return (
    <div className={styles.header}>
      <div className={styles.logoBody}>
        <div className={styles.divider} />
        <Logo isPremium={aiChatContext.isPremiumUser} />
      </div>
      <div className={styles.actions}>
        {canStartNewConversation && (
          <Button
            fab
            kind='plain-faint'
            aria-label={newChatButtonLabel}
            title={newChatButtonLabel}
            onClick={() => location.href = "/"}
          >
            <Icon name='edit-box' />
          </Button>
        )}
      </div>
    </div>
  )
}
