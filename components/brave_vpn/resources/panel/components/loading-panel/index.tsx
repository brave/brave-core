// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { getLocale } from '../../../../../common/locale'
import * as S from './style'
import { LoaderIcon } from 'brave-ui/components/icons'

function LoadingPanel () {
  return (
    <S.Box>
      <S.PanelContent>
        <S.LoaderIconBox>
          <LoaderIcon />
        </S.LoaderIconBox>
        <S.Title>{getLocale('braveVpnLoading')}</S.Title>
      </S.PanelContent>
    </S.Box>
  )
}

export default LoadingPanel
