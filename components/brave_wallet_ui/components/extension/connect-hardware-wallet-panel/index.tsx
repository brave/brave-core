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
import useInterval from '../../../common/hooks/interval'
import { HardwareWalletResponseCodeType } from '../../../common/hardware/types'

export interface Props {
  onCancel: () => void
  walletName: string
  hardwareWalletCode?: HardwareWalletResponseCodeType
  retryCallable: () => void
  onClickInstructions: () => void
}

function ConnectHardwareWalletPanel (props: Props) {
  const {
    onCancel,
    walletName,
    hardwareWalletCode,
    retryCallable,
    onClickInstructions
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
