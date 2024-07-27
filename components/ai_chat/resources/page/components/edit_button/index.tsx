/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import styles from './style.module.scss'

interface Props {
  onClick: () => void
}

function EditButton (props: Props) {
  return (
    <Button onClick={props.onClick} className={styles.editButton}
      fab
      size='tiny'
      kind='plain-faint'
      title={getLocale('editButtonLabel')}
    >
      <Icon name='edit-pencil' />
    </Button>
  )
}

export default EditButton
