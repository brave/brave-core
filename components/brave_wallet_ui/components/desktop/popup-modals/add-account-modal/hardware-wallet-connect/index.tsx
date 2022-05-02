import * as React from 'react'

import { getLocale } from '../../../../../../common/locale'
import { NavButton } from '../../../../extension'
import { BraveWallet, WalletAccountType, CreateAccountOptionsType } from '../../../../../constants/types'
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
import { HardwareDerivationScheme, LedgerDerivationPaths, FilecoinNetwork, DerivationBatch } from '../../../../../common/hardware/types'
import { HardwareVendor } from '../../../../../common/api/hardware_keyrings'

export interface Props {
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Promise<BraveWallet.HardwareWalletAccount[]>
  onAddHardwareAccounts: (selected: BraveWallet.HardwareWalletAccount[]) => void
  getBalance: (address: string, coin: BraveWallet.CoinType) => Promise<string>
  onChangeFilecoinNetwork: (network: FilecoinNetwork) => void
  preAddedHardwareWalletAccounts: WalletAccountType[]
  selectedAccountType: CreateAccountOptionsType
  selectedNetwork: BraveWallet.NetworkInfo
  filecoinNetwork: FilecoinNetwork
}

export default function (props: Props) {
  const {
    selectedAccountType,
    selectedNetwork,
    filecoinNetwork
  } = props
  const [selectedHardwareWallet, setSelectedHardwareWallet] = React.useState<HardwareVendor>(BraveWallet.LEDGER_HARDWARE_VENDOR)
  const [isConnecting, setIsConnecting] = React.useState<boolean>(false)
  const [accounts, setAccounts] = React.useState<BraveWallet.HardwareWalletAccount[]>([])
  const [selectedDerivationPaths, setSelectedDerivationPaths] = React.useState<string[]>([])
  const [connectionError, setConnectionError] = React.useState<ErrorMessage | undefined>(undefined)
  const [selectedDerivationScheme, setSelectedDerivationScheme] = React.useState<HardwareDerivationScheme>(
    LedgerDerivationPaths.LedgerLive
  )
  const [showAccountsList, setShowAccountsList] = React.useState<boolean>(false)
  const getErrorMessage = (error: any, accountTypeName: string) => {
    if (error.statusCode && error.statusCode === 27404) { // Unknown Error
      return { error: getLocale('braveWalletConnectHardwareInfo2').replace('$1', accountTypeName), userHint: '' }
    }

    if (error.statusCode && (error.statusCode === 27904 || error.statusCode === 26368)) { // INCORRECT_LENGTH or INS_NOT_SUPPORTED
      return { error: error.message, userHint: getLocale('braveWalletConnectHardwareWrongApplicationUserHint') }
    }

    if (!error || !error.message) {
      return { error: getLocale('braveWalletUnknownInternalError'), userHint: '' }
    }

    return { error: error.message, userHint: '' }
  }
  const onFilecoinNetworkChanged = (network: FilecoinNetwork) => {
    props.onChangeFilecoinNetwork(network)
    props.onConnectHardwareWallet({
      hardware: BraveWallet.LEDGER_HARDWARE_VENDOR,
      startIndex: 0,
      stopIndex: DerivationBatch,
      network: network,
      coin: BraveWallet.CoinType.FIL
    }).then((result) => {
      setAccounts(result)
    }).catch((error) => {
      setConnectionError(getErrorMessage(error, selectedAccountType.name))
      setShowAccountsList(false)
    }).finally(
      () => setIsConnecting(false)
    )
  }
  const onChangeDerivationScheme = (scheme: HardwareDerivationScheme) => {
    setSelectedDerivationScheme(scheme)
    setAccounts([])
    props.onConnectHardwareWallet({
      hardware: selectedHardwareWallet,
      startIndex: 0,
      stopIndex: DerivationBatch,
      scheme: scheme,
      coin: selectedAccountType.coin,
      network: filecoinNetwork
    }).then((result) => {
      setAccounts(result)
    }).catch((error) => {
      setConnectionError(getErrorMessage(error, selectedAccountType.name))
      setShowAccountsList(false)
    }).finally(
      () => setIsConnecting(false)
    )
  }

  const getDefaultAccountName = (account: BraveWallet.HardwareWalletAccount) => {
    const index = accounts.findIndex(e => e.address === account.address)
    let schemeString
    switch (selectedDerivationScheme) {
      case LedgerDerivationPaths.Legacy:
        schemeString = ' (Legacy)'
        break
      default:
        schemeString = ''
    }

    return index === 0
      ? `${account.hardwareVendor}${schemeString}`
      : `${account.hardwareVendor} ${index}${schemeString}`
  }

  const onAddAccounts = () => {
    const selectedAccounts = accounts.filter(o => selectedDerivationPaths.includes(o.derivationPath))
    const renamedSelectedAccounts = selectedAccounts
      .map(account => ({ ...account, name: getDefaultAccountName(account) }))
    props.onAddHardwareAccounts(renamedSelectedAccounts)
  }

  const getBalance = (address: string, coin: BraveWallet.CoinType) => {
    return props.getBalance(address, coin)
  }

  const selectVendor = (vendor: HardwareVendor) => {
    const derivationPathsEnum = HardwareWalletDerivationPathsMapping[vendor]
    setSelectedDerivationScheme(Object.values(derivationPathsEnum)[0] as HardwareDerivationScheme)
    setSelectedHardwareWallet(vendor)
  }

  const onSelectLedger = () => {
    if (selectedHardwareWallet !== BraveWallet.LEDGER_HARDWARE_VENDOR) {
      setConnectionError(undefined)
    }

    selectVendor(BraveWallet.LEDGER_HARDWARE_VENDOR)
  }

  const onSelectTrezor = () => {
    if (selectedHardwareWallet !== BraveWallet.TREZOR_HARDWARE_VENDOR) {
      setConnectionError(undefined)
    }

    selectVendor(BraveWallet.TREZOR_HARDWARE_VENDOR)
  }

  const onSubmit = () => {
    setConnectionError(undefined)
    setIsConnecting(true)
    props.onConnectHardwareWallet({
      hardware: selectedHardwareWallet,
      startIndex: accounts.length,
      stopIndex: accounts.length + DerivationBatch,
      scheme: selectedDerivationScheme,
      coin: selectedAccountType.coin,
      network: filecoinNetwork
    }).then((result) => {
      setAccounts([...accounts, ...result])
      setShowAccountsList(true)
    }).catch((error) => {
      setConnectionError(getErrorMessage(error, selectedAccountType.name))
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
        preAddedHardwareWalletAccounts={props.preAddedHardwareWalletAccounts}
        onLoadMore={onSubmit}
        selectedDerivationPaths={selectedDerivationPaths}
        setSelectedDerivationPaths={setSelectedDerivationPaths}
        selectedDerivationScheme={selectedDerivationScheme}
        setSelectedDerivationScheme={onChangeDerivationScheme}
        onAddAccounts={onAddAccounts}
        getBalance={getBalance}
        selectedNetwork={selectedNetwork}
        filecoinNetwork={filecoinNetwork}
        onChangeFilecoinNetwork={onFilecoinNetworkChanged}
        selectedAccountType={selectedAccountType}
      />
    )
  }

  return (
    <>
      {(selectedAccountType.coin !== BraveWallet.CoinType.FIL) &&
        <>
          <HardwareTitle>{getLocale('braveWalletConnectHardwareTitle')}</HardwareTitle>
          <HardwareButtonRow>
            <HardwareButton
              onClick={onSelectLedger}
              isSelected={selectedHardwareWallet === BraveWallet.LEDGER_HARDWARE_VENDOR}
              disabled={isConnecting && selectedHardwareWallet !== BraveWallet.LEDGER_HARDWARE_VENDOR}
            >
              <LedgerIcon />
            </HardwareButton>

            <HardwareButton
              onClick={onSelectTrezor}
              isSelected={selectedHardwareWallet === BraveWallet.TREZOR_HARDWARE_VENDOR}
              disabled={isConnecting && selectedHardwareWallet !== BraveWallet.TREZOR_HARDWARE_VENDOR}
            >
              <TrezorIcon />
            </HardwareButton>
          </HardwareButtonRow>
        </>
      }
      <HardwareInfoRow>
        <InfoIcon />
        <HardwareInfoColumn>
          <DisclaimerText>
            {getLocale('braveWalletConnectHardwareInfo1').replace('$1', selectedHardwareWallet)}
          </DisclaimerText>
          <DisclaimerText>{getLocale('braveWalletConnectHardwareInfo2').replace('$1', selectedAccountType.name)}</DisclaimerText>
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
