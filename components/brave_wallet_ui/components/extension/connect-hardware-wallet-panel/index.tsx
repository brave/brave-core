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
import { HardwareWalletErrorType } from '../../../constants/types'
import useInterval from '../../../common/hooks/interval'

export interface Props {
  onCancel: () => void
  walletName: string
  hardwareWalletError?: HardwareWalletErrorType
  retryCallable: () => void
}

function ConnectHardwareWalletPanel (props: Props) {
  const {
    onCancel,
    walletName,
    hardwareWalletError,
    retryCallable
  } = props

  const isConnected = hardwareWalletError !== undefined && hardwareWalletError !== 'deviceNotConnected'
  const requestingConfirmation = hardwareWalletError === 'deviceBusy'

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
      <Title>
        {
          requestingConfirmation
            ? getLocale('braveWalletConnectHardwarePanelConfirmation').replace('$1', walletName)
            : getLocale('braveWalletConnectHardwarePanelConnect').replace('$1', walletName)
        }
      </Title>
      <InstructionsButton onClick={onClickInstructions}>{getLocale('braveWalletConnectHardwarePanelInstructions')}</InstructionsButton>
      <PageIcon />
      <ButtonWrapper>
        <NavButton buttonType='secondary' text={getLocale('braveWalletBackupButtonCancel')} onSubmit={onCancel} />
      </ButtonWrapper>
    </StyledWrapper>
  )
}

export default ConnectHardwareWalletPanel
