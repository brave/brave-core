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
