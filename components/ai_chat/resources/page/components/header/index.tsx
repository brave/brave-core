/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'
import { Conversation } from '../../../common/mojom'
import getAPI from '../../api'
import { useGetVisibleConversation } from '../../hooks/useIsConversationVisible'
import { useActiveChat } from '../../state/active_chat_context'
import { useAIChat, useIsSmall } from '../../state/ai_chat_context'
import { useConversation, useSupportsAttachments } from '../../state/conversation_context'
import FeatureButtonMenu, { Props as FeatureButtonMenuProps } from '../feature_button_menu'
import styles from './style.module.scss'

const Logo = ({ isPremium }: { isPremium: boolean }) => <div className={styles.logo}>
  <Icon name='product-brave-leo' />
  <div className={styles.logoTitle}>Leo AI</div>
  {isPremium && (
    <div className={styles.badgePremium}>PREMIUM</div>
  )}
</div>

const getTitle = (activeConversation?: Conversation) => activeConversation?.title
  || getLocale('conversationListUntitled')

const newChatButtonLabel = getLocale('newChatButtonLabel')
const closeButtonLabel = getLocale('closeLabel')
const openFullPageButtonLabel = getLocale('openFullPageLabel')

export const ConversationHeader = React.forwardRef(function (props: FeatureButtonMenuProps, ref: React.Ref<HTMLDivElement>) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()
  const { selectedConversationId, createNewConversation } = useActiveChat()
  const isMobile = useIsSmall() && aiChatContext.isMobile

  const shouldDisplayEraseAction = (!aiChatContext.isStandalone || isMobile) &&
    conversationContext.conversationHistory.length >= 1

  const activeConversation = useGetVisibleConversation(conversationContext.conversationUuid)

  // Show title when we're on standalone page or not on a default conversation
  const showTitle = (aiChatContext.isStandalone || !!selectedConversationId) && !isMobile
  // Back button shows when we are in a UI which can go back to a "default" conversation
  // (i.e. not a standalone UI).

  const showBackButton = !aiChatContext.isStandalone && showTitle

  const canShowFullScreenButton = aiChatContext.isHistoryFeatureEnabled && !isMobile && !aiChatContext.isStandalone && conversationContext.conversationUuid
  const supportsAttachments = useSupportsAttachments()

  return (
    <div className={styles.header} ref={ref}>
      {showTitle ? (
        <div className={styles.conversationTitle}>
          {showBackButton && <Button
            kind='plain-faint'
            fab
            onClick={() => location.href = '/'}
            title={getLocale('goBackToActiveConversationButton')}
          >
            <Icon name='arrow-left' />
          </Button>}
          <div className={styles.conversationTitleText} title={getTitle(activeConversation)}>{getTitle(activeConversation)}</div>
        </div>
      )
        : (isMobile && aiChatContext.isStandalone
          ? <>
            <Button
              fab
              kind='plain-faint'
              onClick={aiChatContext.toggleSidebar}
            >
              <Icon name='hamburger-menu' />
            </Button>
            <div className={styles.logoBody}>
              <div className={styles.divider} />
              <Logo isPremium={aiChatContext.isPremiumUser} />
            </div>
          </>
          : <Logo isPremium={aiChatContext.isPremiumUser} />
        )}
      <div className={styles.actions}>
        {aiChatContext.hasAcceptedAgreement && (
          <>
            {shouldDisplayEraseAction && (
              <Button
                fab
                kind='plain-faint'
                aria-label={newChatButtonLabel}
                title={newChatButtonLabel}
                onClick={createNewConversation}
              >
                <Icon name={aiChatContext.isHistoryFeatureEnabled ? 'edit-box' : 'erase'} />
              </Button>
            )}
            {canShowFullScreenButton &&
              <Button
                fab
                kind='plain-faint'
                aria-label={openFullPageButtonLabel}
                title={openFullPageButtonLabel}
                onClick={() => getAPI().uiHandler.openConversationFullPage(conversationContext.conversationUuid!)}
              >
                <Icon name='expand' />
              </Button>}
            {supportsAttachments && aiChatContext.tabs.length > 0 && <Button
              fab
              kind={conversationContext.showAttachments ? 'plain' : 'plain-faint'}
              aria-label={getLocale('attachmentsTitle')}
              title={getLocale('attachmentsTitle')}
              onClick={() => conversationContext.setShowAttachments(!conversationContext.showAttachments)}
            >
              <Icon name='attachment' />
            </Button>}
            <FeatureButtonMenu {...props} />
            {!aiChatContext.isStandalone &&
              <Button
                fab
                kind='plain-faint'
                aria-label={closeButtonLabel}
                title={closeButtonLabel}
                className={styles.closeButton}
                onClick={() => getAPI().uiHandler.closeUI()}
              >
                <Icon name='close' />
              </Button>
            }
          </>
        )}
      </div>
    </div>
  )
})

export function NavigationHeader() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()
  const { createNewConversation } = useActiveChat()

  const canStartNewConversation = conversationContext.conversationHistory.length >= 1
    && aiChatContext.hasAcceptedAgreement
  const isMobile = useIsSmall() && aiChatContext.isMobile

  return (
    <div className={styles.header}>
      {isMobile && <Button
        fab
        kind='plain-faint'
        onClick={aiChatContext.toggleSidebar}
      >
        <Icon name='hamburger-menu' />
      </Button>}
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
            onClick={createNewConversation}
          >
            <Icon name='edit-box' />
          </Button>
        )}
      </div>
    </div>
  )
}
