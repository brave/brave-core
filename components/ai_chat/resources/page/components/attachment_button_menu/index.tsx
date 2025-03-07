/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { ConversationContext } from '../../state/conversation_context'

// Utils
import { getLocale } from '$web-common/locale'

// Styles
import styles from './style.module.scss'

type Props = Pick<ConversationContext, 'uploadImage' | 'conversationHistory'>

export default function AttachmentButtonMenu(props: Props) {
  const isMenuDisabled = props.conversationHistory.some(
    (turn) => turn.uploadedImages
  )
  return (
    <>
      <ButtonMenu>
        <div slot='anchor-content'>
          <Button
            fab
            kind='plain-faint'
            title={getLocale('attachmentMenuButtonLabel')}
            isDisabled={isMenuDisabled}
          >
            <Icon name='attachment' />
          </Button>
        </div>
        <leo-menu-item onClick={props.uploadImage}>
          <div className={styles.buttonContent}>
            <Icon
              className={styles.buttonIcon}
              name='upload'
            />
            {getLocale('uploadFileButtonLabel')}
          </div>
        </leo-menu-item>
      </ButtonMenu>
    </>
  )
}
