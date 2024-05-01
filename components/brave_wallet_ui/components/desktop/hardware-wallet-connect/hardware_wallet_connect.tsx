// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../common/locale'
import { onConnectHardwareWallet } from '../../../common/async/hardware'

// components
import { HardwareWalletAccountsList } from './accounts_list'
import {
  AuthorizeHardwareDeviceIFrame //
} from '../../shared/authorize-hardware-device/authorize-hardware-device'
import {
  HardwareButton //
} from '../popup-modals/add-account-modal/hardware-button/hardware_button'

// Styled Components
import {
  HardwareWalletGraphic,
  Instructions,
  Divider
} from './hardware_wallet_connect.styles'
import { Column, VerticalSpace } from '../../shared/style'
import {
  ContinueButton //
} from '../../../page/screens/onboarding/onboarding.style'

// Custom types
import {
  ErrorMessage,
  HardwareWalletDerivationPathsMapping
} from './hardware_wallet_connect.types'
import {
  HardwareDerivationScheme,
  LedgerDerivationPaths,
  DerivationBatchSize,
  SolDerivationPaths
} from '../../../common/hardware/types'
import { HardwareVendor } from '../../../common/api/hardware_keyrings'
import {
  BraveWallet,
  CreateAccountOptionsType,
  FilecoinNetwork
} from '../../../constants/types'
import { LedgerError } from '../../../common/hardware/ledgerjs/ledger-messages'

// hooks
import { useAccountsQuery } from '../../../common/slices/api.slice.extra'
import {
  useImportHardwareAccountsMutation //
} from '../../../common/slices/api.slice'

export interface Props {
  selectedAccountType: CreateAccountOptionsType
  onSelectVendor?: (vendor: HardwareVendor) => void
  onSuccess: () => void
}

const getErrorMessage = (
  error: undefined | string | LedgerError,
  accountTypeName: string
) => {
  if (typeof error === 'undefined') {
    return { error: getLocale('braveWalletUnknownInternalError'), userHint: '' }
  }

  if (typeof error === 'string') {
    return { error: error, userHint: '' }
  }

  // error must be of type LedgerError
  if (
    error.statusCode &&
    (error.statusCode === 27404 || error.statusCode === 21781)
  ) {
    // Unknown Error
    return {
      error: getLocale('braveWalletConnectHardwareInfo2').replace(
        '$1',
        accountTypeName
      ),
      userHint: ''
    }
  }

  if (!error.message) {
    return { error: getLocale('braveWalletUnknownInternalError'), userHint: '' }
  }

  if (
    error.statusCode &&
    (error.statusCode === 27904 || error.statusCode === 26368)
  ) {
    // INCORRECT_LENGTH or INS_NOT_SUPPORTED
    return {
      error: error.message,
      userHint: getLocale('braveWalletConnectHardwareWrongApplicationUserHint')
    }
  }

  return { error: error.message, userHint: '' }
}

export const HardwareWalletConnect = ({
  selectedAccountType,
  onSuccess,
  onSelectVendor
}: Props) => {
  // queries
  const { accounts: savedAccounts } = useAccountsQuery()

  // mutations
  const [importHardwareAccounts] = useImportHardwareAccountsMutation()

  // state
  const [selectedHardwareWallet, setSelectedHardwareWallet] =
    React.useState<HardwareVendor>()
  const [isConnecting, setIsConnecting] = React.useState<boolean>(false)
  const [accounts, setAccounts] = React.useState<
    BraveWallet.HardwareWalletAccount[]
  >([])
  const [selectedDerivationPaths, setSelectedDerivationPaths] = React.useState<
    string[]
  >([])
  const [connectionError, setConnectionError] = React.useState<
    ErrorMessage | undefined
  >(undefined)
  const [selectedDerivationScheme, setSelectedDerivationScheme] =
    React.useState<HardwareDerivationScheme>(
      selectedAccountType.coin === BraveWallet.CoinType.SOL
        ? SolDerivationPaths.Default
        : LedgerDerivationPaths.LedgerLive
    )
  const [showAccountsList, setShowAccountsList] = React.useState<boolean>(false)
  const [filecoinNetwork, setFilecoinNetwork] =
    React.useState<FilecoinNetwork>('f')
  const [showAuthorizeDevice, setShowAuthorizeDevice] =
    React.useState<boolean>(false)
  const hideAuthorizeDevice = () => setShowAuthorizeDevice(false)

  // methods
  const onFilecoinNetworkChanged = React.useCallback(
    (network: FilecoinNetwork) => {
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
      })
        .then((result) => {
          setAccounts(result)
        })
        .catch((error) => {
          setConnectionError(getErrorMessage(error, selectedAccountType.name))
          setShowAccountsList(false)
        })
        .finally(() => setIsConnecting(false))
    },
    [selectedAccountType]
  )

  const onAddHardwareAccounts = React.useCallback(
    async (accounts: BraveWallet.HardwareWalletAccount[]) => {
      try {
        await importHardwareAccounts(accounts).unwrap()
        onSuccess()
      } catch (error) {
        console.log(error)
      }
    },
    [onSuccess, importHardwareAccounts]
  )

  const onChangeDerivationScheme = React.useCallback(
    (scheme: HardwareDerivationScheme) => {
      if (!selectedHardwareWallet) return

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
      })
        .then((result) => {
          setAccounts(result)
        })
        .catch((error) => {
          setConnectionError(getErrorMessage(error, selectedAccountType.name))
          setShowAccountsList(false)
        })
        .finally(() => setIsConnecting(false))
    },
    [selectedHardwareWallet, selectedAccountType, filecoinNetwork]
  )

  const getDefaultAccountName = React.useCallback(
    (account: BraveWallet.HardwareWalletAccount) => {
      const index = accounts.findIndex((e) => e.address === account.address)
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
    },
    [accounts, selectedDerivationScheme]
  )

  const onAddAccounts = React.useCallback(() => {
    const selectedAccounts = accounts.filter((account) =>
      selectedDerivationPaths.includes(account.derivationPath)
    )
    const renamedSelectedAccounts = selectedAccounts.map((account) => ({
      ...account,
      name: getDefaultAccountName(account)
    }))
    onAddHardwareAccounts(renamedSelectedAccounts)
  }, [
    accounts,
    selectedDerivationPaths,
    getDefaultAccountName,
    onAddHardwareAccounts
  ])

  const selectVendor = React.useCallback(
    (vendor: HardwareVendor) => {
      const derivationPathsEnum = HardwareWalletDerivationPathsMapping[vendor]
      setSelectedDerivationScheme(
        Object.values(derivationPathsEnum)[0] as HardwareDerivationScheme
      )
      setSelectedHardwareWallet(vendor)
      onSelectVendor?.(vendor)
    },
    [onSelectVendor]
  )

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
    if (!selectedHardwareWallet) return
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
    })
      .then((result) => {
        setAccounts([...accounts, ...result])
        setShowAccountsList(true)
      })
      .catch((error) => {
        if (error === 'unauthorized') {
          setShowAuthorizeDevice(true)
          setShowAccountsList(false)
        } else {
          setConnectionError(getErrorMessage(error, selectedAccountType.name))
          setShowAccountsList(false)
        }
      })
      .finally(() => setIsConnecting(false))
  }, [
    selectedHardwareWallet,
    accounts,
    selectedDerivationScheme,
    selectedAccountType,
    filecoinNetwork
  ])

  // memos
  const preAddedHardwareWalletAccounts = React.useMemo(() => {
    return savedAccounts.filter(
      (account) => account.accountId.kind === BraveWallet.AccountKind.kHardware
    )
  }, [savedAccounts])

  // render
  if (!selectedHardwareWallet) {
    return (
      <>
        <HardwareButton
          title={getLocale('braveWalletConnectHardwareLedger')}
          description={getLocale(
            'braveWalletConnectHardwareDeviceDescription'
          ).replace('$1', getLocale('braveWalletConnectHardwareLedger'))}
          onClick={onSelectLedger}
        />
        <Divider />
        <HardwareButton
          title={getLocale('braveWalletConnectHardwareTrezor')}
          description={getLocale(
            'braveWalletConnectHardwareDeviceDescription'
          ).replace('$1', getLocale('braveWalletConnectHardwareTrezor'))}
          onClick={onSelectTrezor}
        />{' '}
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
        filecoinNetwork={filecoinNetwork}
        onChangeFilecoinNetwork={onFilecoinNetworkChanged}
        coin={selectedAccountType.coin}
      />
    )
  }

  return (
    <Column>
      <HardwareWalletGraphic hardwareVendor={selectedHardwareWallet} />
      <VerticalSpace space='32px' />
      <Instructions mode={connectionError ? 'error' : 'info'}>
        {connectionError
          ? `${connectionError.error} ${connectionError?.userHint}`
          : getLocale('braveWalletConnectHardwareAuthorizationNeeded').replace(
              '$1',
              selectedHardwareWallet
            )}
      </Instructions>
      <VerticalSpace space='100px' />
      {showAuthorizeDevice ? (
        <AuthorizeHardwareDeviceIFrame coinType={selectedAccountType.coin} />
      ) : (
        <ContinueButton
          onClick={onSubmit}
          isLoading={isConnecting}
        >
          <div slot='loading'>
            {getLocale('braveWalletConnectingHardwareWallet')}
          </div>
          {!isConnecting && getLocale('braveWalletAddAccountConnect')}
        </ContinueButton>
      )}
    </Column>
  )
}

export default HardwareWalletConnect
