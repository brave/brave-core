/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { ConversationContext } from '../../state/conversation_context'
import { MAX_IMAGES } from '../../../common/constants'
import { AIChatContext } from '../../state/ai_chat_context'
import { getImageFiles } from '../../../common/conversation_history_utils'

// Utils
import { getLocale } from '$web-common/locale'

// Styles
import styles from './style.module.scss'

type Props = Pick<ConversationContext, 'uploadImage' | 'getScreenshots' |
  'conversationHistory' | 'associatedContentInfo' | 'setShowAttachments'
  | 'associateDefaultContent'> &
  Pick<AIChatContext, 'isMobile' | 'tabs'> & {
    conversationStarted: boolean
  }

export default function AttachmentButtonMenu(props: Props) {
  const totalUploadedImages = props.conversationHistory.reduce(
    (total, turn) => total +
      (getImageFiles(turn.uploadedFiles)?.length || 0),
    0
  )

  const isMenuDisabled = totalUploadedImages >= MAX_IMAGES
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
        <leo-menu-item onClick={() => props.uploadImage(false)}>
          <div className={styles.buttonContent}>
            <Icon
              className={styles.buttonIcon}
              name='upload'
            />
            {getLocale(S.AI_CHAT_UPLOAD_FILE_BUTTON_LABEL)}
          </div>
        </leo-menu-item>
        {hasAssociatedContent &&
          <leo-menu-item onClick={() => props.getScreenshots()}>
            <div className={styles.buttonContent}>
              <Icon
                className={styles.buttonIcon}
                name='screenshot'
              />
              {getLocale(S.AI_CHAT_SCREENSHOT_BUTTON_LABEL)}
            </div>
          </leo-menu-item>}
        {props.isMobile &&
          <leo-menu-item onClick={() => props.uploadImage(true)}>
            <div className={styles.buttonContent}>
              <Icon
                className={styles.buttonIcon}
                name='camera'
              />
              {getLocale(S.AI_CHAT_TAKE_A_PICTURE_BUTTON_LABEL)}
            </div>
          </leo-menu-item>
        }
        {!props.conversationStarted && <>
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
          {props.tabs.length > 0 && (<leo-menu-item onClick={() => props.setShowAttachments(true)}>
            <div className={styles.buttonContent}>
              <Icon
                className={styles.buttonIcon}
                name='window-tabs'
              />
              {getLocale(S.AI_CHAT_ATTACH_OPEN_TABS_BUTTON_LABEL)}
            </div>
          </leo-menu-item>)}
        </>}
      </ButtonMenu>
    </>
  )
}
