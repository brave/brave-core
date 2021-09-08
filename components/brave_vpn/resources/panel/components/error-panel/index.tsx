import * as React from 'react'
import * as S from './style'
import { Button } from 'brave-ui'
import { AlertCircleIcon } from 'brave-ui/components/icons'
import locale from '../../constants/locale'

interface Props {
  onTryAgainClick: Function
  onChooseServerClick: Function
  region: string
}

function ErrorPanel (props: Props) {
  const handleTryAgain = () => props.onTryAgainClick()
  const handleChooseServer = () => props.onChooseServerClick()

  return (
    <S.Box>
      <S.PanelContent>
        <S.IconBox>
          <AlertCircleIcon color='#84889C' />
        </S.IconBox>
        <S.ReasonTitle>{locale.cantConnectError}</S.ReasonTitle>
        <S.ReasonDesc>Brave Firewall + VPN couldn't connect to the {props.region} server.
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
