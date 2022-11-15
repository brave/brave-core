// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { CloseIcon } from '../icons/close_icon'
import { ShieldsIcon } from '../icons/shields_icon'
import { getLocale } from '../../../../../common/locale'
import Button from '$web-components/button'

import * as S from './style'

interface Props {
  onEnable: () => void
  onDismiss: () => void
  onDecline: () => void
  onAnimationComplete: () => void
}

function MainPanel (props: Props) {
  const [enabled, setEnabled] = React.useState(false)

  const onRootMounted = (elem: HTMLElement | null) => {
    if (elem) {
      elem.style.setProperty('--available-height', screen.availHeight + 'px')
    }
  }

  const onEnablePressed = () => {
    setEnabled(true)
    setTimeout(() => { props.onAnimationComplete() }, 3500)
    props.onEnable()
  }

  return (
    <S.Root ref={onRootMounted} className={enabled ? 'success' : ''}>
      <S.TitleBar>
        <div><ShieldsIcon /></div>
        <S.TitleBarText>{getLocale('cookieListTitle')}</S.TitleBarText>
        <button onClick={props.onDismiss}><CloseIcon /></button>
      </S.TitleBar>
      <S.Content>
        <S.CookieGraphic />
        <S.Header>
          {getLocale('cookieListHeader')}
        </S.Header>
        <S.Description>
          {getLocale('cookieListText')}
        </S.Description>
        <S.OptIn className='opt-in-action'>
          <Button
            isPrimary
            isCallToAction
            scale='large'
            onClick={onEnablePressed}
          >
            {getLocale('cookieListButtonText')}
          </Button>
        </S.OptIn>
        <S.Decline>
          <button onClick={props.onDecline}>
            {getLocale('cookieListNoThanks')}
          </button>
        </S.Decline>
      </S.Content>
      {enabled && <S.Animation />}
    </S.Root>
  )
}

export default MainPanel
