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
  onEditQuestionClicked: () => void
  onCopyQuestionClicked: () => void
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
        {conversationContext.canSubmitUserEntries && (
          <leo-menu-item onClick={props.onEditQuestionClicked}>
            <Icon name='edit-pencil' />
            <span>{getLocale('editPromptButtonLabel')}</span>
          </leo-menu-item>
        )}
        <leo-menu-item onClick={props.onCopyQuestionClicked}>
          <Icon name='copy' />
          <span>{getLocale('copyPromptButtonLabel')}</span>
        </leo-menu-item>
      </ButtonMenu>
    </>
  )
}
