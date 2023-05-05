// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import ReaderModeSVG from '../../svg/reader_mode'
import CloseSVG from '../../svg/close'
import { getLocale } from '$web-common/locale'

interface ReaderModeControlProps {
  onClick?: Function
}

function ReaderModeControl (props: ReaderModeControlProps) {
  return (
    <S.Box>
      <span></span>
      <S.Caption>
        <ReaderModeSVG />
        {getLocale('braveReaderModeCaption')}
      </S.Caption>
      <S.Button>
        <CloseSVG />
      </S.Button>
    </S.Box>
  )
}

export default ReaderModeControl
