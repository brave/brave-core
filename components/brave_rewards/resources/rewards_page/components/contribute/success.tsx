/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { useLocaleContext } from '../../lib/locale_strings'

import { style } from './success.style'

interface Props {
  onClose: () => void
}

export function Success(props: Props) {
  const { getString } = useLocaleContext()
  return (
    <div {...style}>
      <div className='message'>
        <div className='title'>{getString('contributeSuccessTitle')}</div>
        <h4>{getString('contributeSuccessText')}</h4>
      </div>
      <Button kind='outline' onClick={props.onClose}>
        {getString('closeButtonLabel')}
      </Button>
    </div>
  )
}
