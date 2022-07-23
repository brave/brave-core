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
import { BraveWallet } from '../../../constants/types'
import { HardwareWalletResponseCodeType } from '../../../common/hardware/types'

export interface Props {
  onCancel: (accountAddress: string, coinType: BraveWallet.CoinType) => void
  walletName: string
  accountAddress: string
  coinType: BraveWallet.CoinType
  hardwareWalletCode: HardwareWalletResponseCodeType | undefined
  retryCallable: () => void
  onClickInstructions: () => void
}

function ConnectHardwareWalletPanel (props: Props) {
  const {
    onCancel,
    walletName,
    accountAddress,
    coinType,
    hardwareWalletCode,
    retryCallable,
    onClickInstructions
  } = props

  const isConnected = React.useMemo((): boolean => {
    return hardwareWalletCode !== 'deviceNotConnected'
  }, [hardwareWalletCode])

  const title = React.useMemo(() => {
    if (hardwareWalletCode === 'deviceBusy') {
      return getLocale('braveWalletConnectHardwarePanelConfirmation')
    }

    if (hardwareWalletCode === 'openLedgerApp') {
      let network = (coinType === BraveWallet.CoinType.SOL) ? 'Solana' : 'Ethereum'
      return getLocale('braveWalletConnectHardwarePanelOpenApp')
        .replace('$1', network)
        .replace('$2', walletName)
    }

    return getLocale('braveWalletConnectHardwarePanelConnect').replace('$1', walletName)
  }, [hardwareWalletCode])

  const onCancelConnect = () => {
    onCancel(accountAddress, coinType)
  }

  useInterval(retryCallable, 3000, !isConnected ? 5000 : null)

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
            <NavButton buttonType='secondary' text={getLocale('braveWalletBackupButtonCancel')} onSubmit={onCancelConnect} />
          </ButtonWrapper>
        )
      }

    </StyledWrapper>
  )
}

export default ConnectHardwareWalletPanel
