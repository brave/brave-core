import * as React from 'react'

import locale from '../../../../../constants/locale'
import { NavButton } from '../../../../extension'
import * as Result from '../../../../../common/types/result'

// Styled Components
import { DisclaimerText, InfoIcon } from '../style'
import {
  ConnectingButton,
  ConnectingButtonText,
  HardwareButton,
  HardwareButtonRow,
  HardwareInfoColumn,
  HardwareInfoRow,
  HardwareTitle,
  LedgerIcon,
  TrezorIcon
} from './style'

// Custom types
import { HardwareWallet, HardwareWalletAccount, HardwareWalletConnectOpts, LedgerDerivationPaths } from './types'

import HardwareWalletAccountsList from './accounts-list'

export interface Props {
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Result.Type<HardwareWalletAccount[]>
}

const derivationBatch = 4

export default function (props: Props) {
  const [selectedHardwareWallet, setSelectedHardwareWallet] = React.useState<HardwareWallet>(HardwareWallet.Ledger)
  const [isConnecting, setIsConnecting] = React.useState<boolean>(false)
  const [accounts, setAccounts] = React.useState<Array<HardwareWalletAccount>>([])
  const [selectedDerivationPaths, setSelectedDerivationPaths] = React.useState<string[]>([])
  const [connectionError, setConnectionError] = React.useState<string>('')
  const [selectedDerivationScheme, setSelectedDerivationScheme] = React.useState<string>(
    LedgerDerivationPaths.LedgerLive.toString()
  )

  const onConnectHardwareWallet = (hardware: HardwareWallet) => {
    setIsConnecting(true)

    const result = props.onConnectHardwareWallet({
      hardware,
      startIndex: accounts.length,
      stopIndex: accounts.length + derivationBatch
    })

    if (Result.isError(result)) {
      setConnectionError(result.message)
    } else {
      setAccounts([...accounts, ...result])
    }

    setIsConnecting(false)
  }

  const onAddAccounts = () => {
    // TODO: create view-only hardware wallet accounts for each item in derivationPaths.
    console.log(`Add accounts:`, selectedDerivationPaths)
  }

  const onSelectLedger = () => {
    setSelectedHardwareWallet(HardwareWallet.Ledger)
  }

  const onSelectTrezor = () => {
    setSelectedHardwareWallet(HardwareWallet.Trezor)
  }

  const onSubmit = () => onConnectHardwareWallet(selectedHardwareWallet)

  if (connectionError !== '') {
    console.error(connectionError)
  }

  if (accounts.length > 0) {
    return (
      <HardwareWalletAccountsList
        hardwareWallet={selectedHardwareWallet}
        accounts={accounts}
        onLoadMore={onSubmit}
        selectedDerivationPaths={selectedDerivationPaths}
        setSelectedDerivationPaths={setSelectedDerivationPaths}
        selectedDerivationScheme={selectedDerivationScheme}
        setSelectedDerivationScheme={setSelectedDerivationScheme}
        onAddAccounts={onAddAccounts}
      />
    )
  }

  return (
    <>
      <HardwareTitle>{locale.connectHardwareTitle}</HardwareTitle>
      <HardwareButtonRow>
        <HardwareButton onClick={onSelectLedger} isSelected={selectedHardwareWallet === HardwareWallet.Ledger}>
          <LedgerIcon />
        </HardwareButton>
        <HardwareButton onClick={onSelectTrezor} isSelected={selectedHardwareWallet === HardwareWallet.Trezor}>
          <TrezorIcon />
        </HardwareButton>
      </HardwareButtonRow>
      <HardwareInfoRow>
        <InfoIcon />
        <HardwareInfoColumn>
          <DisclaimerText>
            {locale.connectHardwareInfo1} {selectedHardwareWallet} {locale.connectHardwareInfo2}
          </DisclaimerText>
          <DisclaimerText>{locale.connectHardwareInfo3}</DisclaimerText>
        </HardwareInfoColumn>
      </HardwareInfoRow>

      {isConnecting ? (
        <ConnectingButton>
          <ConnectingButtonText>{locale.connectingHardwareWallet}</ConnectingButtonText>
        </ConnectingButton>
      ) : (
        <NavButton onSubmit={onSubmit} text={locale.addAccountConnect} buttonType='primary' />
      )}
    </>
  )
}
