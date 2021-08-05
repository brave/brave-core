// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '../../../../../common/locale'
import BraveTalkIcon from './braveTalkIcon'
import CloseIcon from './closeIcon'
import { LinkButton } from '../../../outlineButton'
import * as S from './style'
import { braveTalkWidgetUrl } from '../../../../constants/new_tab_ui'

type Props = React.PropsWithChildren<{
  onClose: () => any
}>

const BraveTalkTooltip = React.forwardRef<HTMLDivElement, Props>(function BraveTalkTooltip (props, ref) {
  return (
    <S.Anchor ref={ref}>
      <S.Tooltip>
        <S.Title>
          <S.TitleIcon><BraveTalkIcon /></S.TitleIcon>
          {getLocale('braveTalkPromptTitle')}
        </S.Title>
        <S.Body>
          {getLocale('braveTalkPromptDescription')}
        </S.Body>
        <LinkButton href={braveTalkWidgetUrl} onClick={props.onClose}>
          {getLocale('braveTalkPromptAction')}
        </LinkButton>
        <S.CloseButton
          onClick={props.onClose}
          aria-label={getLocale('close')}
        >
          <CloseIcon />
        </S.CloseButton>
      </S.Tooltip>
      {props.children}
    </S.Anchor>
  )
})

export default BraveTalkTooltip
