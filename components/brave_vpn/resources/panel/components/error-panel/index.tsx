import * as React from 'react'
import { Button } from 'brave-ui'

import * as S from './style'
import { AlertCircleIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../../common/locale'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'

function ErrorPanel () {
  const dispatch = useDispatch()
  const currentRegion = useSelector(state => state.currentRegion)

  const handleTryAgain = () => {
    dispatch(Actions.retryConnect())
  }

  const handleChooseServer = () => {
    dispatch(Actions.toggleRegionSelector(true))
  }

  const matches = {
    $1: getLocale('braveVpn'),
    $2: currentRegion?.namePretty || ''
  }

  return (
    <S.Box>
      <S.PanelContent>
        <S.IconBox>
          <AlertCircleIcon color='#84889C' />
        </S.IconBox>
        <S.ReasonTitle>{getLocale('braveVpnUnableConnectToServer')}</S.ReasonTitle>
        <S.ReasonDesc>
          {getLocale('braveVpnUnableConnectInfo').replace(/\$\d+/g, (match) => matches[match])}
        </S.ReasonDesc>
        <S.ActionArea>
            <Button
              level='primary'
              type='accent'
              brand='rewards'
              text={getLocale('braveVpnTryAgain')}
              onClick={handleTryAgain}
            />
            <Button
              level='tertiary'
              type='accent'
              brand='rewards'
              text={getLocale('braveVpnChooseAnotherServer')}
              onClick={handleChooseServer}
            />
        </S.ActionArea>
      </S.PanelContent>
    </S.Box>
  )
}

export default ErrorPanel
