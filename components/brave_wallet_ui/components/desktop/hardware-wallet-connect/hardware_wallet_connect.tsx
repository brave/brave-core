// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as crypto from 'crypto'
import { assert, assertNotReached } from 'chrome://resources/js/assert.js'

// utils
import { getLocale } from '../../../../common/locale'
import { loadAccountsFromDevice } from '../../../common/async/hardware'

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
  DerivationBatchSize,
  DerivationScheme,
  AllHardwareImportSchemes,
  HardwareImportScheme,
  AccountFromDevice
} from '../../../common/hardware/types'
import { BraveWallet, CreateAccountOptionsType } from '../../../constants/types'
import { LedgerError } from '../../../common/hardware/ledgerjs/ledger-messages'

// hooks
import { useAccountsQuery } from '../../../common/slices/api.slice.extra'
import {
  useImportHardwareAccountsMutation //
} from '../../../common/slices/api.slice'

export interface Props {
  selectedAccountType: CreateAccountOptionsType
  onSelectVendor?: (vendor: BraveWallet.HardwareVendor) => void
  onSuccess: () => void
}

const vendorName = (vendor: BraveWallet.HardwareVendor) => {
  switch (vendor) {
    case BraveWallet.HardwareVendor.kLedger:
      return getLocale('braveWalletConnectHardwareLedger')
    case BraveWallet.HardwareVendor.kTrezor:
      return getLocale('braveWalletConnectHardwareTrezor')
    default:
      assertNotReached(`Unknown vendor ${vendor}`)
  }
}

interface ErrorMessage {
  error: string
  userHint: string
}

const getErrorMessage = (
  error: undefined | string | LedgerError,
  accountTypeName: string
): ErrorMessage => {
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

const defaultSchemeForCoinAndVendor = (
  coin: BraveWallet.CoinType,
  vendor: BraveWallet.HardwareVendor
) => {
  const result = AllHardwareImportSchemes.find(
    (details) => details.coin === coin && details.vendor === vendor
  )

  assert(result, `Not supported ${coin} and ${vendor}`)
  return result.derivationScheme
}

const getDefaultAccountName = (scheme: HardwareImportScheme, index: number) => {
  let schemeString
  switch (scheme.derivationScheme) {
    case DerivationScheme.EthLedgerLegacy:
      schemeString = ' (Legacy)'
      break
    default:
      schemeString = ''
  }
  return index === 0
    ? `${vendorName(scheme.vendor)}${schemeString}`
    : `${vendorName(scheme.vendor)} ${index}${schemeString}`
}

const hardwareDeviceIdFromAddress = (address: string) => {
  return crypto.createHash('sha256').update(address).digest('hex')
}

const findHardwareImportScheme = (scheme: DerivationScheme) => {
  const result = AllHardwareImportSchemes.find(
    (d) => d.derivationScheme === scheme
  )
  assert(result)
  return result
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
  const [selectedHardwareVendor, setSelectedHardwareVendor] =
    React.useState<BraveWallet.HardwareVendor>()
  const [isConnecting, setIsConnecting] = React.useState<boolean>(false)
  const [accounts, setAccounts] = React.useState<
    Array<Required<AccountFromDevice>>
  >([])
  const [connectionError, setConnectionError] = React.useState<
    ErrorMessage | undefined
  >(undefined)
  const [currentDerivationScheme, setCurrentDerivationScheme] =
    React.useState<DerivationScheme>(DerivationScheme.EthLedgerLive)
  const [showAccountsList, setShowAccountsList] = React.useState<boolean>(false)
  const [showAuthorizeDevice, setShowAuthorizeDevice] =
    React.useState<boolean>(false)
  const hideAuthorizeDevice = () => setShowAuthorizeDevice(false)
  const [numberOfAccountsToLoad, setNumberOfAccountsToLoad] =
    React.useState<number>(0)

  const currentHardwareImportScheme = findHardwareImportScheme(
    currentDerivationScheme
  )

  const setHardwareImportScheme = (scheme: DerivationScheme) => {
    if (currentDerivationScheme === scheme) {
      return
    }
    setAccounts([])
    setNumberOfAccountsToLoad(DerivationBatchSize)
    setCurrentDerivationScheme(scheme)
  }

  const onAccountChecked = (path: string, checked: boolean) => {
    const newAccounts = accounts.map((acc) => {
      if (acc.derivationPath === path) {
        return { ...acc, shouldAddToWallet: checked }
      }
      return acc
    })
    setAccounts(newAccounts)
  }

  const loadMoreAccounts = React.useCallback(async () => {
    if (numberOfAccountsToLoad <= accounts.length) {
      return
    }

    setIsConnecting(true)
    const result = await loadAccountsFromDevice({
      startIndex: accounts.length,
      count: numberOfAccountsToLoad - accounts.length,
      scheme: currentHardwareImportScheme,
      onAuthorized: hideAuthorizeDevice
    })
    setIsConnecting(false)

    if (result.payload) {
      setShowAccountsList(true)

      const newAccounts: Array<Required<AccountFromDevice>> =
        result.payload.map((acc) => {
          const alreadyInWallet = !!savedAccounts.find((a) => {
            return (
              a.accountId.kind === BraveWallet.AccountKind.kHardware &&
              a.address === acc.address
            )
          })
          return { ...acc, alreadyInWallet, shouldAddToWallet: false }
        })

      setAccounts([...accounts, ...newAccounts])
    } else {
      setShowAccountsList(false)
      if (result.error === 'unauthorized') {
        setShowAuthorizeDevice(true)
      } else {
        setConnectionError(
          getErrorMessage(result.error, selectedAccountType.name)
        )
      }
    }
  }, [
    currentHardwareImportScheme,
    numberOfAccountsToLoad,
    savedAccounts,
    accounts,
    selectedAccountType.name
  ])

  React.useEffect(() => {
    loadMoreAccounts()
  }, [currentHardwareImportScheme, numberOfAccountsToLoad, loadMoreAccounts])

  const onAddAccounts = React.useCallback(async () => {
    if (accounts.length === 0) {
      return
    }
    const deviceId = hardwareDeviceIdFromAddress(accounts[0].address)
    const hwAccounts: BraveWallet.HardwareWalletAccount[] = accounts
      .filter((account) => account.shouldAddToWallet)
      .map((account, index) => ({
        address: account.address,
        derivationPath: account.derivationPath,
        name: getDefaultAccountName(currentHardwareImportScheme, index),
        hardwareVendor: currentHardwareImportScheme.vendor,
        deviceId: deviceId,
        coin: currentHardwareImportScheme.coin,
        keyringId: currentHardwareImportScheme.keyringId
      }))

    try {
      await importHardwareAccounts(hwAccounts).unwrap()
      onSuccess()
    } catch (error) {
      console.log(error)
    }
  }, [currentHardwareImportScheme, accounts, onSuccess, importHardwareAccounts])

  const selectVendor = React.useCallback(
    (vendor: BraveWallet.HardwareVendor) => {
      setCurrentDerivationScheme(
        defaultSchemeForCoinAndVendor(selectedAccountType.coin, vendor)
      )
      setSelectedHardwareVendor(vendor)
      onSelectVendor?.(vendor)
    },
    [onSelectVendor, selectedAccountType.coin]
  )

  const onSelectLedger = React.useCallback(() => {
    if (selectedHardwareVendor !== BraveWallet.HardwareVendor.kLedger) {
      setConnectionError(undefined)
    }

    selectVendor(BraveWallet.HardwareVendor.kLedger)
  }, [selectedHardwareVendor, selectVendor])

  const onSelectTrezor = React.useCallback(() => {
    if (selectedHardwareVendor !== BraveWallet.HardwareVendor.kTrezor) {
      setConnectionError(undefined)
    }

    selectVendor(BraveWallet.HardwareVendor.kTrezor)
  }, [selectedHardwareVendor, selectVendor])

  const increaseNumberOfAccounts = () => {
    if (currentHardwareImportScheme.singleAccount) {
      setNumberOfAccountsToLoad(1)
      return
    }
    setNumberOfAccountsToLoad((curState) => curState + DerivationBatchSize)
  }

  const trezorEnabled = [BraveWallet.CoinType.ETH].includes(
    selectedAccountType.coin
  )

  // render
  if (!selectedHardwareVendor) {
    return (
      <>
        <HardwareButton
          title={getLocale('braveWalletConnectHardwareLedger')}
          description={getLocale(
            'braveWalletConnectHardwareDeviceDescription'
          ).replace('$1', getLocale('braveWalletConnectHardwareLedger'))}
          onClick={onSelectLedger}
        />
        {trezorEnabled && (
          <>
            <Divider />
            <HardwareButton
              title={getLocale('braveWalletConnectHardwareTrezor')}
              description={getLocale(
                'braveWalletConnectHardwareDeviceDescription'
              ).replace('$1', getLocale('braveWalletConnectHardwareTrezor'))}
              onClick={onSelectTrezor}
            />
          </>
        )}
      </>
    )
  }

  if (showAccountsList) {
    return (
      <HardwareWalletAccountsList
        currentHardwareImportScheme={currentHardwareImportScheme}
        setHardwareImportScheme={setHardwareImportScheme}
        accounts={accounts}
        onLoadMore={increaseNumberOfAccounts}
        onAccountChecked={onAccountChecked}
        onAddAccounts={onAddAccounts}
      />
    )
  }

  return (
    <Column>
      <HardwareWalletGraphic hardwareVendor={selectedHardwareVendor} />
      <VerticalSpace space='32px' />
      <Instructions mode={connectionError ? 'error' : 'info'}>
        {connectionError
          ? `${connectionError.error} ${connectionError?.userHint}`
          : getLocale('braveWalletConnectHardwareAuthorizationNeeded').replace(
              '$1',
              vendorName(selectedHardwareVendor)
            )}
      </Instructions>
      <VerticalSpace space='100px' />
      {showAuthorizeDevice ? (
        <AuthorizeHardwareDeviceIFrame coinType={selectedAccountType.coin} />
      ) : (
        <ContinueButton
          onClick={increaseNumberOfAccounts}
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
