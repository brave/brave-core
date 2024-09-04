/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useLocaleContext } from '../../lib/locale_strings'

import { style } from './transfer_error.style'

interface Props {
  onClose: () => void
}

export function TransferError(props: Readonly<Props>) {
  const { getString } = useLocaleContext()
  return (
    <div {...style}>
      <div className='message'>
        <Icon name='warning-circle-filled' />
        <h3>{getString('contributeErrorTitle')}</h3>
        <p>{getString('contributeErrorText')}</p>
      </div>
      <Button kind='outline' onClick={props.onClose}>
        {getString('closeButtonLabel')}
      </Button>
    </div>
  )
}
