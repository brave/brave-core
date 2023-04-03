// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import ReaderModeSVG from '../../svg/readerMode'
import CloseSVG from '../../svg/close'

interface ReaderModeControlProps {
  onClick?: Function
}

function ReaderModeControl (props: ReaderModeControlProps) {
  return (
    <S.Box>
      <span></span>
      <S.Caption>
        <ReaderModeSVG />
        Reader mode
      </S.Caption>
      <S.Button>
        <CloseSVG />
      </S.Button>
    </S.Box>
  )
}

export default ReaderModeControl
