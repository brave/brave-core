/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { ConversationContext } from '../../state/conversation_context'
import { AIChatContext } from '../../state/ai_chat_context'
import { shouldDisableAttachmentsButton } from '../../../common/conversation_history_utils'

// Utils
import { getLocale } from '$web-common/locale'

// Styles
import styles from './style.module.scss'

type Props = Pick<
  ConversationContext,
  | 'uploadFile'
  | 'getScreenshots'
  | 'conversationHistory'
  | 'associatedContentInfo'
  | 'setAttachmentsDialog'
  | 'associateDefaultContent'
  | 'unassociatedTabs'
>
  & Pick<AIChatContext, 'isMobile'> & {
    conversationStarted: boolean
  }

export default function AttachmentButtonMenu(props: Props) {
  const isMenuDisabled = shouldDisableAttachmentsButton(
    props.conversationHistory,
  )
  const hasAssociatedContent = props.associatedContentInfo.length > 0

  return (
    <>
      <ButtonMenu>
        <div
          slot='anchor-content'
          className={styles.anchor}
        >
          <Button
            fab
            kind='plain-faint'
            title={getLocale(S.AI_CHAT_LEO_ATTACHMENT_MENU_BUTTON_LABEL)}
            isDisabled={isMenuDisabled}
          >
            <Icon name='attachment' />
          </Button>
        </div>
        <leo-menu-item onClick={() => props.uploadFile(false)}>
          <div className={styles.buttonContent}>
            <Icon
              className={styles.buttonIcon}
              name='upload'
            />
            {getLocale(S.AI_CHAT_UPLOAD_FILE_BUTTON_LABEL)}
          </div>
        </leo-menu-item>
        {hasAssociatedContent && (
          <leo-menu-item onClick={() => props.getScreenshots()}>
            <div className={styles.buttonContent}>
              <Icon
                className={styles.buttonIcon}
                name='screenshot'
              />
              {getLocale(S.AI_CHAT_SCREENSHOT_BUTTON_LABEL)}
            </div>
          </leo-menu-item>
        )}
        {props.isMobile && (
          <leo-menu-item onClick={() => props.uploadFile(true)}>
            <div className={styles.buttonContent}>
              <Icon
                className={styles.buttonIcon}
                name='camera'
              />
              {getLocale(S.AI_CHAT_TAKE_A_PICTURE_BUTTON_LABEL)}
            </div>
          </leo-menu-item>
        )}
        {props.associateDefaultContent && (
          <leo-menu-item onClick={() => props.associateDefaultContent?.()}>
            <div className={styles.buttonContent}>
              <Icon
                className={styles.buttonIcon}
                name='window-tab'
              />
              {getLocale(S.AI_CHAT_CURRENT_TAB_CONTENTS_BUTTON_LABEL)}
            </div>
          </leo-menu-item>
        )}
        {props.unassociatedTabs.length > 0 && (
          <leo-menu-item onClick={() => props.setAttachmentsDialog('tabs')}>
            <div className={styles.buttonContent}>
              <Icon
                className={styles.buttonIcon}
                name='window-tabs'
              />
              {getLocale(S.AI_CHAT_ATTACH_OPEN_TABS_BUTTON_LABEL)}
            </div>
          </leo-menu-item>
        )}
        <leo-menu-item onClick={() => props.setAttachmentsDialog('bookmarks')}>
          <div className={styles.buttonContent}>
            <Icon
              className={styles.buttonIcon}
              name='window-bookmark'
            />
            {getLocale(S.AI_CHAT_ATTACH_OPEN_BOOKMARKS_BUTTON_LABEL)}
          </div>
        </leo-menu-item>
      </ButtonMenu>
    </>
  )
}
