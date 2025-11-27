/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
// import * as ReactDOM from 'react-dom'
// import { showAlert } from '@brave/leo/react/alertCenter'
import Button from '@brave/leo/react/button'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import classnames from '$web-common/classnames'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './style.module.scss'

interface ContextMenuHumanProps {
  isOpen: boolean
  onClick: () => void
  onClose: () => void
  onEditQuestionClicked?: () => void
  onCopyQuestionClicked?: () => void
  onSaveAsSkillClicked?: () => void
}

export default function ContextMenuHuman(props: ContextMenuHumanProps) {
  const conversationContext = useUntrustedConversationContext()

  return (
    <>
      <ButtonMenu
        className={styles.buttonMenu}
        isOpen={props.isOpen}
        onClose={props.onClose}
      >
        <Button
          fab
          slot='anchor-content'
          size='tiny'
          kind='plain-faint'
          onClick={props.onClick}
          className={classnames({
            [styles.moreButton]: true,
            [styles.moreButtonActive]: props.isOpen,
            [styles.moreButtonHide]: conversationContext.isMobile,
          })}
        >
          <Icon name='more-vertical' />
        </Button>
        {conversationContext.canSubmitUserEntries
          && props.onEditQuestionClicked && (
            <leo-menu-item onClick={props.onEditQuestionClicked}>
              <Icon name='edit-pencil' />
              <span>{getLocale(S.CHAT_UI_EDIT_PROMPT_BUTTON_LABEL)}</span>
            </leo-menu-item>
          )}
        {props.onCopyQuestionClicked && (
          <leo-menu-item onClick={props.onCopyQuestionClicked}>
            <Icon name='copy' />
            <span>{getLocale(S.CHAT_UI_COPY_PROMPT_BUTTON_LABEL)}</span>
          </leo-menu-item>
        )}
        {props.onSaveAsSkillClicked && (
          <leo-menu-item onClick={props.onSaveAsSkillClicked}>
            <Icon name='slash' />
            <span>{getLocale(S.CHAT_UI_SAVE_AS_SKILL_BUTTON_LABEL)}</span>
          </leo-menu-item>
        )}
      </ButtonMenu>
    </>
  )
}
