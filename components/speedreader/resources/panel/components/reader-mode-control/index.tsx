// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import { SiteSettings, dataHandler } from '../../api/browser'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'

interface ReaderModeControlProps {
  siteSettings: SiteSettings
  onClick?: Function
}

function ReaderModeControl (props: ReaderModeControlProps) {
  return (
    <S.Box>
      <span></span>
      <S.Caption>
        {!props.siteSettings.speedreaderEnabled && <Icon name='product-readermode' />}
        {props.siteSettings.speedreaderEnabled && <Icon name='product-speedreader' />}
        {getLocale('braveReaderModeCaption')}
      </S.Caption>
      <S.Button onClick={() => { dataHandler.viewOriginal() }}>
        <Icon name='close' />
      </S.Button> 
    </S.Box>
  )
}

export default ReaderModeControl
