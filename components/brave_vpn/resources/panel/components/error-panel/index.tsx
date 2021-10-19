import * as React from 'react'
import { Button } from 'brave-ui'

import * as S from './style'
import { AlertCircleIcon } from 'brave-ui/components/icons'
import locale from '../../constants/locale'
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

  return (
    <S.Box>
      <S.PanelContent>
        <S.IconBox>
          <AlertCircleIcon color='#84889C' />
        </S.IconBox>
        <S.ReasonTitle>{locale.cantConnectError}</S.ReasonTitle>
        <S.ReasonDesc>Brave Firewall + VPN couldn't connect to the{' '}
          {currentRegion?.namePretty} server.
          You can try again, or choose another.</S.ReasonDesc>
        <S.ActionArea>
            <Button
              level='primary'
              type='accent'
              brand='rewards'
              text={locale.tryAgain}
              onClick={handleTryAgain}
            />
            <Button
              level='tertiary'
              type='accent'
              brand='rewards'
              text={locale.chooseAnotherServer}
              onClick={handleChooseServer}
            />
        </S.ActionArea>
      </S.PanelContent>
    </S.Box>
  )
}

export default ErrorPanel
