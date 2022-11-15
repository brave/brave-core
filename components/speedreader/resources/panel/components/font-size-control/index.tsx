// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import PlusSVG from '../../svg/plus-icon'
import MinusSVG from '../../svg/minus-icon'
import { FontSize } from '../../api/browser'

enum ActionType {
  Inc,
  Dec
}

interface FontSizeControlProps {
  currentSize: FontSize
  onClick?: Function
}

function FontSizeControl (props: FontSizeControlProps) {
  const updateSize = (action: ActionType) => {
    const newSize = action === ActionType.Dec ? props.currentSize - 10 : props.currentSize + 10
    if (newSize >= FontSize.MIN_VALUE && newSize <= FontSize.MAX_VALUE) {
      props.onClick?.(newSize)
      return
    }
    return props.onClick?.(props.currentSize)
  }

  return (
    <S.Box>
      <S.ButtonLeft
        onClick={() => updateSize(ActionType.Dec)}
      >
        <MinusSVG />
      </S.ButtonLeft>
      <span>{props.currentSize}%</span>
      <S.ButtonRight
        onClick={() => updateSize(ActionType.Inc)}
      >
        <PlusSVG />
      </S.ButtonRight>
    </S.Box>
  )
}

export default FontSizeControl
