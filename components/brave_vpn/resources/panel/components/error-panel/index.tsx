import * as React from 'react'
import Button from '$web-components/button'

import * as S from './style'
import { AlertCircleIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../../common/locale'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'

interface Props {
  showContactSupport: React.MouseEventHandler<HTMLButtonElement>
}

function ErrorPanel (props: Props) {
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
            type={'submit'}
            isPrimary
            isCallToAction
            onClick={handleTryAgain}
          >
            {getLocale('braveVpnTryAgain')}
          </Button>
          <S.ButtonText onClick={handleChooseServer}>
            {getLocale('braveVpnChooseAnotherServer')}
          </S.ButtonText>
          <S.ButtonText onClick={props.showContactSupport}>
            {getLocale('braveVpnContactSupport')}
          </S.ButtonText>
        </S.ActionArea>
      </S.PanelContent>
    </S.Box>
  )
}

export default ErrorPanel
