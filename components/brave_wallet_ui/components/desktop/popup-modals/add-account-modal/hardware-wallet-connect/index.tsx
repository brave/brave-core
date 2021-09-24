import * as React from 'react'

import locale from '../../../../../constants/locale'
import { NavButton } from '../../../../extension'

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
  TrezorIcon,
  ErrorText
} from './style'

// Custom types
import { HardwareWalletAccount, HardwareWalletConnectOpts, LedgerDerivationPaths } from './types'

import {
  kLedgerHardwareVendor,
  kTrezorHardwareVendor
} from '../../../../../constants/types'

import HardwareWalletAccountsList from './accounts-list'

export interface Props {
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Promise<HardwareWalletAccount[]>
  onAddHardwareAccounts: (selected: HardwareWalletAccount[]) => void
  getBalance: (address: string) => Promise<string>
}

const derivationBatch = 4

export default function (props: Props) {
  const [selectedHardwareWallet, setSelectedHardwareWallet] = React.useState<string>(kLedgerHardwareVendor)
  const [isConnecting, setIsConnecting] = React.useState<boolean>(false)
  const [accounts, setAccounts] = React.useState<Array<HardwareWalletAccount>>([])
  const [selectedDerivationPaths, setSelectedDerivationPaths] = React.useState<string[]>([])
  const [connectionError, setConnectionError] = React.useState<string>('')
  const [selectedDerivationScheme, setSelectedDerivationScheme] = React.useState<string>(
    LedgerDerivationPaths.LedgerLive.toString()
  )

  const getErrorMessage = (error: any) => {
    if (error.statusCode && error.statusCode === 27404) {
      return locale.connectHardwareInfo3
    }
    if (!error || !error.message) {
      return locale.unknownInternalError
    }
    return error.message
  }

  const onSelectedDerivationScheme = (scheme: string) => {
    setSelectedDerivationScheme(scheme)
    setAccounts([])
    props.onConnectHardwareWallet({
      hardware: selectedHardwareWallet,
      startIndex: 0,
      stopIndex: derivationBatch,
      scheme: scheme
    }).then((result) => {
      setAccounts(result)
    }).catch((error) => {
      setConnectionError(getErrorMessage(error))
    })
  }
  const onConnectHardwareWallet = (hardware: string) => {
    setIsConnecting(true)

    props.onConnectHardwareWallet({
      hardware,
      startIndex: accounts.length,
      stopIndex: accounts.length + derivationBatch,
      scheme: selectedDerivationScheme
    }).then((result) => {
      setAccounts([...accounts, ...result])
      setIsConnecting(false)
    }).catch((error) => {
      setConnectionError(getErrorMessage(error))
      setIsConnecting(false)
    })
  }

  const onAddAccounts = () => {
    const selectedAccounts = accounts.filter(o => selectedDerivationPaths.includes(o.derivationPath))
    props.onAddHardwareAccounts(selectedAccounts)
  }

  const getBalance = (address: string) => {
    return props.getBalance(address)
  }

  const onSelectLedger = () => {
    setSelectedHardwareWallet(kLedgerHardwareVendor)
  }

  const onSelectTrezor = () => {
    setSelectedHardwareWallet(kTrezorHardwareVendor)
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
        setSelectedDerivationScheme={onSelectedDerivationScheme}
        onAddAccounts={onAddAccounts}
        getBalance={getBalance}
      />
    )
  }

  return (
    <>
      <HardwareTitle>{locale.connectHardwareTitle}</HardwareTitle>
      <HardwareButtonRow>
        <HardwareButton onClick={onSelectLedger} isSelected={selectedHardwareWallet === kLedgerHardwareVendor}>
          <LedgerIcon />
        </HardwareButton>
        <HardwareButton onClick={onSelectTrezor} isSelected={selectedHardwareWallet === kTrezorHardwareVendor}>
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
      {connectionError !== '' &&
              <ErrorText>{connectionError}</ErrorText>
            }

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
