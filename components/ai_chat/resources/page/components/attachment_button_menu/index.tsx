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

type Props = Pick<
  ConversationContext,
  'uploadImage'
>

export default function AttachmentButtonMenu(props: Props) {
  // State
  const [isMenuOpen, setIsMenuOpen] = React.useState<boolean>(false)
  const [file, setFile] = React.useState<HTMLInputElement['files']>()
  const inputRef = React.useRef<HTMLInputElement | null>(null)

  // This effect can be removed when ready, just here for testing.
  React.useEffect(() => {
    console.log(file)
  }, [file])

  // Methods
  const handleFileChange = React.useCallback(
    (file: React.ChangeEvent<HTMLInputElement>) => {
      if (file.target.files) {
        // Just for testing integration, probably don't need to
        // set this file to sate, but instead send to server API.
        setFile(file.target.files)
      }
    },
    []
  )

  return (
    <>
      <input
        type='file'
        ref={inputRef}
        style={{ display: 'none' }}
        onChange={handleFileChange}
      />
      <ButtonMenu
        id='attachMenu'
        isOpen={isMenuOpen}
        onChange={({ isOpen }) => setIsMenuOpen(isOpen)}
        onClose={() => setIsMenuOpen(false)}
      >
        <Button
          fab
          kind='plain-faint'
          title={getLocale('attachmentMenuButtonLabel')}
          slot='anchor-content'
        >
          <Icon name='attachment' />
        </Button>
        <Button
          fab
          kind='plain-faint'
          title={getLocale('attachmentMenuButtonLabel')}
          slot='anchor-content'
          onClick={props.uploadImage}
        >
        <Icon name='attachment' />
        </Button>
        <div className={styles.section}>
          <Icon
            className={styles.attachIcon}
            name='attachment'
          />
          {getLocale('attachMenuTitle')}
        </div>
        <leo-menu-item key='upload' onClick={props.uploadImage}>
          {getLocale('uploadFileButtonLabel')}
        </leo-menu-item>
        <leo-menu-item
          // Needs onClick method
          onClick={() => {}}
        >
          {getLocale('screenshotButtonLabel')}
        </leo-menu-item>
        <leo-menu-item
          // Needs onClick method
          onClick={() => {}}
        >
          {getLocale('currentTabContentsButtonLabel')}
        </leo-menu-item>
      </ButtonMenu>
    </>
  )
}
