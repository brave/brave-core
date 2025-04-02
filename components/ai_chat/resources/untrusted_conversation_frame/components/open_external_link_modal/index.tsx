/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'
import Dialog from '@brave/leo/react/dialog'
import { getLocale } from '$web-common/locale'
import styles from './style.module.scss'

interface OpenExternalLinkModalProps {
  isOpen: boolean
  onOpen: (ingnoreChecked: boolean) => void
  onClose: () => void
}

export default function OpenExternalLinkModal(
  props: OpenExternalLinkModalProps
) {
  const { isOpen, onOpen, onClose } = props
  const [ingnoreChecked, setIngnoreChecked] = React.useState(false)
  return (
    <Dialog
      isOpen={isOpen}
      showClose
      onClose={onClose}
      className={styles.dialog}
    >
      <div
        slot='title'
        className={styles.dialogTitle}
      >
        {getLocale('openExternalLink')}
      </div>
      <div className={styles.description}>
        {getLocale('openExternalLinkInfo')}
        <Checkbox
          checked={ingnoreChecked}
          onChange={({ checked }) => setIngnoreChecked(checked)}
        >
          <span>{getLocale('openExternalLinkCheckboxLabel')}</span>
        </Checkbox>
      </div>
      <div
        slot='actions'
        className={styles.actionsRow}
      >
        <div className={styles.buttonsWrapper}>
          <Button
            kind='plain-faint'
            size='medium'
            onClick={onClose}
          >
            {getLocale('cancelButtonLabel')}
          </Button>
          <Button
            kind='filled'
            size='medium'
            onClick={() => onOpen(ingnoreChecked)}
          >
            {getLocale('openLabel')}
          </Button>
        </div>
      </div>
    </Dialog>
  )
}
