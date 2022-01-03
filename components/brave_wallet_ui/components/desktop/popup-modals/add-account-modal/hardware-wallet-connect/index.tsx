import * as React from 'react'

import { getLocale } from '../../../../../../common/locale'
import { NavButton } from '../../../../extension'
import { TREZOR_HARDWARE_VENDOR, LEDGER_HARDWARE_VENDOR } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
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
  ErrorText,
  LoadIcon
} from './style'
// Custom types
import { HardwareWalletConnectOpts, ErrorMessage, HardwareWalletDerivationPathsMapping } from './types'
import HardwareWalletAccountsList from './accounts-list'
import { HardwareDerivationScheme, HardwareWalletAccount, LedgerDerivationPaths } from '../../../../../common/hardware/types'
import { HardwareVendor } from '../../../../../common/api/hardware_keyrings'

export interface Props {
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Promise<HardwareWalletAccount[]>
  onAddHardwareAccounts: (selected: HardwareWalletAccount[]) => void
  getBalance: (address: string) => Promise<string>
}

const derivationBatch = 4

export default function (props: Props) {
  const [selectedHardwareWallet, setSelectedHardwareWallet] = React.useState<HardwareVendor>(LEDGER_HARDWARE_VENDOR)
  const [isConnecting, setIsConnecting] = React.useState<boolean>(false)
  const [accounts, setAccounts] = React.useState<HardwareWalletAccount[]>([])
  const [selectedDerivationPaths, setSelectedDerivationPaths] = React.useState<string[]>([])
  const [connectionError, setConnectionError] = React.useState<ErrorMessage | undefined>(undefined)
  const [selectedDerivationScheme, setSelectedDerivationScheme] = React.useState<HardwareDerivationScheme>(
    LedgerDerivationPaths.LedgerLive
  )
  const [showAccountsList, setShowAccountsList] = React.useState<boolean>(false)

  const getErrorMessage = (error: any) => {
    if (error.statusCode && error.statusCode === 27404) { // Unknown Error
      return { error: getLocale('braveWalletConnectHardwareInfo2'), userHint: '' }
    }

    if (error.statusCode && (error.statusCode === 27904 || error.statusCode === 26368)) { // INCORRECT_LENGTH or INS_NOT_SUPPORTED
      return { error: error.message, userHint: getLocale('braveWalletConnectHardwareWrongApplicationUserHint') }
    }

    if (!error || !error.message) {
      return { error: getLocale('braveWalletUnknownInternalError'), userHint: '' }
    }

    return { error: error.message, userHint: '' }
  }

  const onChangeDerivationScheme = (scheme: HardwareDerivationScheme) => {
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
      setShowAccountsList(false)
    }).finally(
      () => setIsConnecting(false)
    )
  }

  const onAddAccounts = () => {
    const selectedAccounts = accounts.filter(o => selectedDerivationPaths.includes(o.derivationPath))
    props.onAddHardwareAccounts(selectedAccounts)
  }

  const getBalance = (address: string) => {
    return props.getBalance(address)
  }

  const selectVendor = (vendor: HardwareVendor) => {
    const derivationPathsEnum = HardwareWalletDerivationPathsMapping[vendor]
    setSelectedDerivationScheme(Object.values(derivationPathsEnum)[0] as HardwareDerivationScheme)
    setSelectedHardwareWallet(vendor)
  }

  const onSelectLedger = () => {
    if (selectedHardwareWallet !== LEDGER_HARDWARE_VENDOR) {
      setConnectionError(undefined)
    }

    selectVendor(LEDGER_HARDWARE_VENDOR)
  }

  const onSelectTrezor = () => {
    if (selectedHardwareWallet !== TREZOR_HARDWARE_VENDOR) {
      setConnectionError(undefined)
    }

    selectVendor(TREZOR_HARDWARE_VENDOR)
  }

  const onSubmit = () => {
    setConnectionError(undefined)
    setIsConnecting(true)
    props.onConnectHardwareWallet({
      hardware: selectedHardwareWallet,
      startIndex: accounts.length,
      stopIndex: accounts.length + derivationBatch,
      scheme: selectedDerivationScheme
    }).then((result) => {
      setAccounts([...accounts, ...result])
      setShowAccountsList(true)
    }).catch((error) => {
      setConnectionError(getErrorMessage(error))
      setShowAccountsList(false)
    }).finally(
      () => setIsConnecting(false)
    )
  }

  if (showAccountsList) {
    return (
      <HardwareWalletAccountsList
        hardwareWallet={selectedHardwareWallet}
        accounts={accounts}
        onLoadMore={onSubmit}
        selectedDerivationPaths={selectedDerivationPaths}
        setSelectedDerivationPaths={setSelectedDerivationPaths}
        selectedDerivationScheme={selectedDerivationScheme}
        setSelectedDerivationScheme={onChangeDerivationScheme}
        onAddAccounts={onAddAccounts}
        getBalance={getBalance}
      />
    )
  }

  return (
    <>
      <HardwareTitle>{getLocale('braveWalletConnectHardwareTitle')}</HardwareTitle>
      <HardwareButtonRow>
        <HardwareButton
          onClick={onSelectLedger}
          isSelected={selectedHardwareWallet === LEDGER_HARDWARE_VENDOR}
          disabled={isConnecting && selectedHardwareWallet !== LEDGER_HARDWARE_VENDOR}
        >
          <LedgerIcon />
        </HardwareButton>
        <HardwareButton
          onClick={onSelectTrezor}
          isSelected={selectedHardwareWallet === TREZOR_HARDWARE_VENDOR}
          disabled={isConnecting && selectedHardwareWallet !== TREZOR_HARDWARE_VENDOR}
        >
          <TrezorIcon />
        </HardwareButton>
      </HardwareButtonRow>
      <HardwareInfoRow>
        <InfoIcon />
        <HardwareInfoColumn>
          <DisclaimerText>
            {getLocale('braveWalletConnectHardwareInfo1').replace('$1', selectedHardwareWallet)}
          </DisclaimerText>
          <DisclaimerText>{getLocale('braveWalletConnectHardwareInfo2')}</DisclaimerText>
        </HardwareInfoColumn>
      </HardwareInfoRow>
      {connectionError &&
        <>
          <ErrorText>{connectionError.error}</ErrorText>
          <ErrorText>{connectionError.userHint}</ErrorText>
        </>
      }

      {isConnecting ? (
        <ConnectingButton>
          <LoadIcon size='small' />
          <ConnectingButtonText>{getLocale('braveWalletConnectingHardwareWallet')}</ConnectingButtonText>
        </ConnectingButton>
      ) : (
        <NavButton onSubmit={onSubmit} text={getLocale('braveWalletAddAccountConnect')} buttonType='primary' />
      )}
    </>
  )
}
