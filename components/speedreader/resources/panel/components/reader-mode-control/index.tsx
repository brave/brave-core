// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'

interface ReaderModeControlProps {
  onClick?: Function
}

function ReaderModeControl (props: ReaderModeControlProps) {
  return (
    <S.Box>
      <span></span>
      <S.Caption>
        <Icon name='product-readermode' />
        {getLocale('braveReaderModeCaption')}
      </S.Caption>
      <S.Button onClick={() => { props.onClick?.() }}>
        <Icon name='close' />
      </S.Button>
    </S.Box>
  )
}

export default ReaderModeControl
