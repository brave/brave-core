import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description,
  PageIcon,
  InstructionsButton,
  ButtonWrapper,
  Indicator,
  ConnectionRow
} from './style'
import { NavButton } from '..'
import { getLocale } from '../../../../common/locale'
import { HardwareWalletResponseCodeType } from '../../../constants/types'
import useInterval from '../../../common/hooks/interval'

export interface Props {
  onCancel: () => void
  walletName: string
  hardwareWalletCode?: HardwareWalletResponseCodeType
  retryCallable: () => void
}

function ConnectHardwareWalletPanel (props: Props) {
  const {
    onCancel,
    walletName,
    hardwareWalletCode,
    retryCallable
  } = props

  const isConnected = hardwareWalletCode !== undefined && hardwareWalletCode !== 'deviceNotConnected'

  const title = React.useMemo(() => {
    if (hardwareWalletCode === 'deviceBusy') {
      return getLocale('braveWalletConnectHardwarePanelConfirmation').replace('$1', walletName)
    }

    if (hardwareWalletCode === 'openEthereumApp') {
      return getLocale('braveWalletConnectHardwarePanelOpenApp').replace('$1', walletName)
    }

    return getLocale('braveWalletConnectHardwarePanelConnect').replace('$1', walletName)
  }, [hardwareWalletCode])

  const onClickInstructions = () => {
    window.open('https://support.brave.com/hc/en-us/articles/4409309138701', '_blank')
  }

  useInterval(retryCallable, 3000)

  return (
    <StyledWrapper>
      <ConnectionRow>
        <Indicator isConnected={isConnected} />
        <Description>
          {
            isConnected
              ? getLocale('braveWalletConnectHardwarePanelConnected').replace('$1', walletName)
              : getLocale('braveWalletConnectHardwarePanelDisconnected').replace('$1', walletName)
          }
        </Description>
      </ConnectionRow>
      <Title>{title}</Title>
      <InstructionsButton onClick={onClickInstructions}>{getLocale('braveWalletConnectHardwarePanelInstructions')}</InstructionsButton>
      <PageIcon />

      {
        hardwareWalletCode !== 'deviceBusy' && (
          <ButtonWrapper>
            <NavButton buttonType='secondary' text={getLocale('braveWalletBackupButtonCancel')} onSubmit={onCancel} />
          </ButtonWrapper>
        )
      }

    </StyledWrapper>
  )
}

export default ConnectHardwareWalletPanel
