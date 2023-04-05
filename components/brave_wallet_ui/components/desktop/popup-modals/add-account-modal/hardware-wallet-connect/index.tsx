// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'

// utils
import { getLocale } from '../../../../../../common/locale'
import { getBalance } from '../../../../../common/async/lib'

// components
import HardwareWalletAccountsList from './accounts-list'
import { AuthorizeHardwareDeviceIFrame } from '../../../../shared/authorize-hardware-device/authorize-hardware-device'
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
  ErrorText,
  LoadIcon
} from './style'

// Custom types
import { ErrorMessage, HardwareWalletDerivationPathsMapping } from './types'
import { HardwareDerivationScheme, LedgerDerivationPaths, FilecoinNetwork, DerivationBatchSize, SolDerivationPaths } from '../../../../../common/hardware/types'
import { HardwareVendor } from '../../../../../common/api/hardware_keyrings'
import { WalletPageActions } from '../../../../../page/actions'
import { BraveWallet, CreateAccountOptionsType, WalletState } from '../../../../../constants/types'
import { LedgerError } from '../../../../../common/hardware/ledgerjs/ledger-messages'

// hooks
import { useLib } from '../../../../../common/hooks'
import {
  useGetDefaultNetworksQuery,
  useGetSelectedChainQuery
} from '../../../../../common/slices/api.slice'

export interface Props {
  selectedAccountType: CreateAccountOptionsType
  onSuccess: () => void
}

const getErrorMessage = (error: undefined | string | LedgerError, accountTypeName: string) => {
  if (typeof error === 'undefined') {
    return { error: getLocale('braveWalletUnknownInternalError'), userHint: '' }
  }

  if (typeof error === 'string') {
    return { error: error, userHint: '' }
  }

  // error must be of type LedgerError
  if (error.statusCode && error.statusCode === 27404) { // Unknown Error
    return { error: getLocale('braveWalletConnectHardwareInfo2').replace('$1', accountTypeName), userHint: '' }
  }

  if (!error.message) {
    return { error: getLocale('braveWalletUnknownInternalError'), userHint: '' }
  }

  if (error.statusCode && (error.statusCode === 27904 || error.statusCode === 26368)) { // INCORRECT_LENGTH or INS_NOT_SUPPORTED
    return { error: error.message, userHint: getLocale('braveWalletConnectHardwareWrongApplicationUserHint') }
  }

  return { error: error.message, userHint: '' }
}

export const HardwareWalletConnect = ({ onSuccess, selectedAccountType }: Props) => {
  // lib
  const { onConnectHardwareWallet } = useLib()

  // redux
  const dispatch = useDispatch()
  const savedAccounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: defaultNetworks } = useGetDefaultNetworksQuery()

  // state
  const [selectedHardwareWallet, setSelectedHardwareWallet] = React.useState<HardwareVendor>(BraveWallet.LEDGER_HARDWARE_VENDOR)
  const [isConnecting, setIsConnecting] = React.useState<boolean>(false)
  const [accounts, setAccounts] = React.useState<BraveWallet.HardwareWalletAccount[]>([])
  const [selectedDerivationPaths, setSelectedDerivationPaths] = React.useState<string[]>([])
  const [connectionError, setConnectionError] = React.useState<ErrorMessage | undefined>(undefined)
  const [selectedDerivationScheme, setSelectedDerivationScheme] = React.useState<HardwareDerivationScheme>(
    selectedAccountType.coin === BraveWallet.CoinType.SOL ? SolDerivationPaths.Default : LedgerDerivationPaths.LedgerLive
  )
  const [showAccountsList, setShowAccountsList] = React.useState<boolean>(false)
  const [filecoinNetwork, setFilecoinNetwork] = React.useState<FilecoinNetwork>('f')
  const [showAuthorizeDevice, setShowAuthorizeDevice] = React.useState<boolean>(false)
  const hideAuthorizeDevice = () => setShowAuthorizeDevice(false)

  // methods
  const onFilecoinNetworkChanged = React.useCallback((network: FilecoinNetwork) => {
    // clear previous accounts & show loading spinner
    setAccounts([])
    setFilecoinNetwork(network)
    onConnectHardwareWallet({
      hardware: BraveWallet.LEDGER_HARDWARE_VENDOR,
      startIndex: 0,
      stopIndex: DerivationBatchSize,
      network: network,
      coin: BraveWallet.CoinType.FIL,
      onAuthorized: hideAuthorizeDevice
    }).then((result) => {
      setAccounts(result)
    }).catch((error) => {
      setConnectionError(getErrorMessage(error, selectedAccountType.name))
      setShowAccountsList(false)
    }).finally(
      () => setIsConnecting(false)
    )
  }, [onConnectHardwareWallet, selectedAccountType])

  const onAddHardwareAccounts = React.useCallback((selected: BraveWallet.HardwareWalletAccount[]) => {
    dispatch(WalletPageActions.addHardwareAccounts(selected))
    onSuccess()
  }, [onSuccess])

  const onChangeDerivationScheme = React.useCallback((scheme: HardwareDerivationScheme) => {
    setSelectedDerivationScheme(scheme)
    setAccounts([])
    onConnectHardwareWallet({
      hardware: selectedHardwareWallet,
      startIndex: 0,
      stopIndex: DerivationBatchSize,
      scheme: scheme,
      coin: selectedAccountType.coin,
      network: filecoinNetwork,
      onAuthorized: hideAuthorizeDevice
    }).then((result) => {
      setAccounts(result)
    }).catch((error) => {
      setConnectionError(getErrorMessage(error, selectedAccountType.name))
      setShowAccountsList(false)
    }).finally(
      () => setIsConnecting(false)
    )
  }, [onConnectHardwareWallet, selectedHardwareWallet, selectedAccountType, filecoinNetwork])

  const getDefaultAccountName = React.useCallback((account: BraveWallet.HardwareWalletAccount) => {
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
  }, [accounts, selectedDerivationScheme])

  const onAddAccounts = React.useCallback(() => {
    const selectedAccounts = accounts.filter(account => selectedDerivationPaths.includes(account.derivationPath))
    const renamedSelectedAccounts = selectedAccounts
      .map(account => ({ ...account, name: getDefaultAccountName(account) }))
    onAddHardwareAccounts(renamedSelectedAccounts)
  }, [accounts, selectedDerivationPaths, getDefaultAccountName, onAddHardwareAccounts])

  const selectVendor = React.useCallback((vendor: HardwareVendor) => {
    const derivationPathsEnum = HardwareWalletDerivationPathsMapping[vendor]
    setSelectedDerivationScheme(Object.values(derivationPathsEnum)[0] as HardwareDerivationScheme)
    setSelectedHardwareWallet(vendor)
  }, [])

  const onSelectLedger = React.useCallback(() => {
    if (selectedHardwareWallet !== BraveWallet.LEDGER_HARDWARE_VENDOR) {
      setConnectionError(undefined)
    }

    selectVendor(BraveWallet.LEDGER_HARDWARE_VENDOR)
  }, [selectedHardwareWallet, selectVendor])

  const onSelectTrezor = React.useCallback(() => {
    if (selectedHardwareWallet !== BraveWallet.TREZOR_HARDWARE_VENDOR) {
      setConnectionError(undefined)
    }

    selectVendor(BraveWallet.TREZOR_HARDWARE_VENDOR)
  }, [selectedHardwareWallet, selectVendor])

  const onSubmit = React.useCallback(() => {
    setConnectionError(undefined)
    setIsConnecting(true)
    onConnectHardwareWallet({
      hardware: selectedHardwareWallet,
      startIndex: accounts.length,
      stopIndex: accounts.length + DerivationBatchSize,
      scheme: selectedDerivationScheme,
      coin: selectedAccountType.coin,
      network: filecoinNetwork,
      onAuthorized: hideAuthorizeDevice
    }).then((result) => {
      setAccounts([...accounts, ...result])
      setShowAccountsList(true)
    }).catch((error) => {
      if (error === 'unauthorized') {
        setShowAuthorizeDevice(true)
        setShowAccountsList(false)
      } else {
        setConnectionError(getErrorMessage(error, selectedAccountType.name))
        setShowAccountsList(false)
      }
    }).finally(
      () => setIsConnecting(false)
    )
  }, [onConnectHardwareWallet, selectedHardwareWallet, accounts, selectedDerivationScheme, selectedAccountType, filecoinNetwork])

  // memos
  const preAddedHardwareWalletAccounts = React.useMemo(() => {
    return savedAccounts.filter(account =>
      ['Ledger', 'Trezor'].includes(account.accountType)
    )
  }, [savedAccounts])

  const selectedAccountTypesDefaultNetwork = React.useMemo(() => {
    return (
      defaultNetworks?.find(
        (network: BraveWallet.NetworkInfo) =>
          network.coin === selectedAccountType.coin
      ) ?? selectedNetwork
    )
  }, [defaultNetworks, selectedAccountType, selectedNetwork])

  // render
  if (showAuthorizeDevice) {
    return (
      <>
        <HardwareInfoRow>
          <InfoIcon />
          <HardwareInfoColumn>
            <DisclaimerText>
              {getLocale('braveWalletConnectHardwareAuthorizationNeeded')}
            </DisclaimerText>
          </HardwareInfoColumn>
        </HardwareInfoRow>
        <AuthorizeHardwareDeviceIFrame coinType={selectedAccountType.coin} />
      </>
    )
  }

  if (showAccountsList) {
    return (
      <HardwareWalletAccountsList
        hardwareWallet={selectedHardwareWallet}
        accounts={accounts}
        preAddedHardwareWalletAccounts={preAddedHardwareWalletAccounts}
        onLoadMore={onSubmit}
        selectedDerivationPaths={selectedDerivationPaths}
        setSelectedDerivationPaths={setSelectedDerivationPaths}
        selectedDerivationScheme={selectedDerivationScheme}
        setSelectedDerivationScheme={onChangeDerivationScheme}
        onAddAccounts={onAddAccounts}
        getBalance={getBalance}
        selectedNetwork={selectedAccountTypesDefaultNetwork}
        filecoinNetwork={filecoinNetwork}
        onChangeFilecoinNetwork={onFilecoinNetworkChanged}
        selectedAccountType={selectedAccountType}
      />
    )
  }

  return (
    <>
      {(selectedAccountType.coin !== BraveWallet.CoinType.FIL && selectedAccountType.coin !== BraveWallet.CoinType.SOL) &&
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

export default HardwareWalletConnect
