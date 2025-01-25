// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Button from '@brave/leo/react/button'

// Constants
import {
  LOCAL_STORAGE_KEYS //
} from '../../../../../common/constants/local-storage-keys'

// Types
import {
  BraveWallet,
  AddressMessageInfo,
  AddressMessageInfoIds,
  CoinTypesMap,
  AddressHistory
} from '../../../../../constants/types'

// Selectors
import {
  useSafeUISelector //
} from '../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../common/selectors'

// Queries
import {
  useAccountsQuery,
  useGetCombinedTokensListQuery,
  useReceiveAddressQuery
} from '../../../../../common/slices/api.slice.extra'
import {
  useEnableEnsOffchainLookupMutation,
  useGenerateReceiveAddressMutation,
  useGetAddressFromNameServiceUrlQuery,
  useGetEthAddressChecksumQuery,
  useGetFVMAddressQuery,
  useGetIsBase58EncodedSolPubkeyQuery,
  useValidateUnifiedAddressQuery
} from '../../../../../common/slices/api.slice'

// Hooks
import {
  useLocalStorage //
} from '../../../../../common/hooks/use_local_storage'

// Utils
import {
  isValidBtcAddress,
  isValidEVMAddress,
  isValidFilAddress
} from '../../../../../utils/address-utils'
import {
  getAddressHistoryIdentifier //
} from '../../../../../utils/local-storage-utils'

// Messages
import {
  FEVMAddressConvertionMessage,
  InvalidAddressMessage,
  HasNoDomainAddressMessage,
  AddressValidationMessages
} from '../../send-ui-messages'

// Utils
import { getLocale } from '../../../../../../common/locale'
import { endsWithAny } from '../../../../../utils/string-utils'
import {
  findTokenByContractAddress,
  getAssetIdKey
} from '../../../../../utils/asset-utils'
import { isFVMAccount } from '../../../../../utils/account-utils'

// Constants
import {
  supportedENSExtensions,
  supportedSNSExtensions,
  supportedUDExtensions
} from '../../../../../common/constants/domain-extensions'

// Components
import {
  PopupModal //
} from '../../../../../components/desktop/popup-modals/index'
import {
  AddressMessage //
} from '../../components/address-message/address-message'
import {
  AccountListItem //
} from '../account_list_item/account_list_item'
import { ChecksumInfo } from './checksum_info'

// Styled Components
import {
  Row,
  Column,
  VerticalSpace,
  Text
} from '../../../../../components/shared/style'
import {
  Wrapper,
  ScrollContainer,
  LabelText,
  AddressInput,
  AddressButton,
  WalletIcon,
  AddressButtonText,
  DomainLoadIcon,
  SearchBoxContainer,
  TrashIcon
} from './select_address_modal.style'

interface Props {
  onClose: () => void
  selectedNetwork: BraveWallet.NetworkInfo | undefined
  fromAccountId: BraveWallet.AccountId | undefined
  selectedAsset: BraveWallet.BlockchainToken | undefined
  toAddressOrUrl: string
  setToAddressOrUrl: (address: string) => void
  setResolvedDomainAddress: (address: string) => void
}

export const SelectAddressModal = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const {
      onClose,
      fromAccountId,
      selectedNetwork,
      selectedAsset,
      toAddressOrUrl,
      setToAddressOrUrl,
      setResolvedDomainAddress
    } = props

    // UI Selectors (safe)
    const isPanel = useSafeUISelector(UISelectors.isPanel)

    // Local Storage
    const [addressHistory, setAddressHistory] = useLocalStorage<AddressHistory>(
      LOCAL_STORAGE_KEYS.RECENTLY_INTERACTED_WITH_ADDRESS_HISTORY,
      {}
    )
    const [isAddressHistoryDisabled, setIsAddressHistoryDisabled] =
      useLocalStorage<boolean>(
        LOCAL_STORAGE_KEYS.RECENTLY_INTERACTED_WITH_ADDRESS_HISTORY_IS_DISABLED,
        false
      )

    // Mutations
    const [enableEnsOffchainLookup] = useEnableEnsOffchainLookupMutation()
    const [generateReceiveAddress, { isLoading: isGeneratingAddress }] =
      useGenerateReceiveAddressMutation()

    // State
    const [searchValue, setSearchValue] = React.useState<string>(
      toAddressOrUrl ?? ''
    )
    const [showChecksumInfo, setShowChecksumInfo] =
      React.useState<boolean>(false)
    const [isOffChainEnsWarningDismissed, dismissOffchainEnsWarning] =
      React.useState<boolean>(false)

    // Queries
    const { accounts } = useAccountsQuery()
    const { data: fullTokenList } = useGetCombinedTokensListQuery()
    const { receiveAddress: fromAccountAddress } =
      useReceiveAddressQuery(fromAccountId)

    // Domain name lookup Queries
    const selectedSendAssetId = selectedAsset
      ? getAssetIdKey(selectedAsset)
      : null

    const lowercaseSearchValue = searchValue.toLowerCase()

    const searchValueHasValidExtension = lowercaseSearchValue
      ? endsWithAny(supportedUDExtensions, lowercaseSearchValue) ||
        (selectedAsset?.coin === BraveWallet.CoinType.SOL &&
          endsWithAny(supportedSNSExtensions, lowercaseSearchValue)) ||
        (selectedAsset?.coin === BraveWallet.CoinType.ETH &&
          endsWithAny(supportedENSExtensions, lowercaseSearchValue))
      : false

    const {
      data: nameServiceInfo,
      isFetching: isSearchingForDomain,
      isError: hasNameServiceError = false
    } = useGetAddressFromNameServiceUrlQuery(
      searchValueHasValidExtension
        ? {
            tokenId: selectedSendAssetId,
            // preventing additional lookups for address casing changes
            url: lowercaseSearchValue
          }
        : skipToken
    )

    const resolvedDomainAddress =
      searchValueHasValidExtension &&
      !hasNameServiceError &&
      nameServiceInfo?.address
        ? nameServiceInfo.address
        : ''

    const showEnsOffchainWarning =
      nameServiceInfo?.requireOffchainConsent || false

    // Memos
    const accountsByNetwork = React.useMemo(() => {
      if (!selectedNetwork || !fromAccountId) {
        return []
      }

      if (fromAccountId.coin === BraveWallet.CoinType.FIL) {
        const filecoinAccounts = accounts.filter(
          (account) => account.accountId.keyringId === fromAccountId?.keyringId
        )
        const fevmAccounts = accounts.filter(
          (account) => account.accountId.coin === BraveWallet.CoinType.ETH
        )
        return filecoinAccounts.concat(fevmAccounts)
      }

      // TODO(apaymyshev): for bitcoin should allow sending to my account, but
      // from different keyring (i.e. segwit -> taproot)
      // https://github.com/brave/brave-browser/issues/29262
      return accounts.filter(
        (account) =>
          account.accountId.keyringId === fromAccountId.keyringId ||
          (selectedAsset?.contractAddress === '' &&
            isFVMAccount(account, selectedNetwork))
      )
    }, [accounts, selectedNetwork, fromAccountId, selectedAsset])

    const filteredAccounts = React.useMemo(() => {
      return accountsByNetwork.filter(
        (account) =>
          account.accountId.address
            .toLocaleLowerCase()
            .startsWith(searchValue.toLocaleLowerCase()) ||
          account.name.toLowerCase().startsWith(searchValue.toLocaleLowerCase())
      )
    }, [accountsByNetwork, searchValue])

    const addressHistoryIdentifier =
      getAddressHistoryIdentifier(selectedNetwork)

    const addressHistoryByNetworkOrCoin = React.useMemo(() => {
      if (!addressHistoryIdentifier) {
        return []
      }
      return addressHistory[addressHistoryIdentifier] ?? []
    }, [addressHistoryIdentifier, addressHistory])

    const filteredAddressHistory = React.useMemo(() => {
      return addressHistoryByNetworkOrCoin.filter((address) =>
        address.toLocaleLowerCase().startsWith(searchValue.toLocaleLowerCase())
      )
    }, [addressHistoryByNetworkOrCoin, searchValue])

    const evmAddressesforFVMTranslation = React.useMemo(
      () =>
        accountsByNetwork
          .filter(
            (account) => account.accountId.coin === BraveWallet.CoinType.ETH
          )
          .map((account) => account.accountId.address),
      [accountsByNetwork]
    )

    const { data: fevmTranslatedAddresses } = useGetFVMAddressQuery(
      selectedNetwork &&
        selectedNetwork?.coin === BraveWallet.CoinType.FIL &&
        evmAddressesforFVMTranslation.length
        ? {
            coin: selectedNetwork.coin,
            addresses: evmAddressesforFVMTranslation,
            isMainNet: selectedNetwork.chainId === BraveWallet.FILECOIN_MAINNET
          }
        : skipToken
    )

    const trimmedSearchValue = searchValue.trim()
    const isValidEvmAddress = isValidEVMAddress(trimmedSearchValue)

    const { data: ethAddressChecksum = '' } = useGetEthAddressChecksumQuery(
      isValidEvmAddress ? trimmedSearchValue : skipToken
    )

    const showFilecoinFEVMWarning =
      fromAccountId?.coin === BraveWallet.CoinType.FIL
        ? trimmedSearchValue.startsWith('0x') &&
          !validateETHAddress(trimmedSearchValue, ethAddressChecksum)
        : false

    const { data: isBase58 = false } = useGetIsBase58EncodedSolPubkeyQuery(
      fromAccountId?.coin === BraveWallet.CoinType.SOL && trimmedSearchValue
        ? trimmedSearchValue
        : skipToken
    )

    const {
      data: zecAddressValidationResult = BraveWallet
        .ZCashAddressValidationResult.Unknown
    } = useValidateUnifiedAddressQuery(
      fromAccountId?.coin === BraveWallet.CoinType.ZEC && trimmedSearchValue
        ? {
            address: trimmedSearchValue,
            testnet: selectedNetwork?.chainId === BraveWallet.Z_CASH_TESTNET
          }
        : skipToken
    )

    const addressMessageId: AddressMessageInfoIds | undefined =
      React.useMemo(() => {
        return processAddressOrUrl({
          addressOrUrl: trimmedSearchValue,
          fromAccountAddress: fromAccountAddress,
          ethAddressChecksum,
          isBase58,
          coinType: fromAccountId?.coin ?? BraveWallet.CoinType.ETH,
          token: selectedAsset,
          zecAddressValidationResult,
          fullTokenList,
          hasNameServiceError,
          isValidExtension: searchValueHasValidExtension,
          isSearchingForDomain,
          resolvedDomainAddress,
          showEnsOffchainWarning,
          isOffChainEnsWarningDismissed
        })
      }, [
        trimmedSearchValue,
        fromAccountAddress,
        fromAccountId?.coin,
        ethAddressChecksum,
        isBase58,
        selectedAsset,
        zecAddressValidationResult,
        fullTokenList,
        hasNameServiceError,
        searchValueHasValidExtension,
        isSearchingForDomain,
        resolvedDomainAddress,
        showEnsOffchainWarning,
        isOffChainEnsWarningDismissed
      ])

    const addressMessageInformation = React.useMemo(() => {
      return getAddressMessageInfo({
        showFilecoinFEVMWarning,
        fevmTranslatedAddresses,
        toAddressOrUrl: searchValue,
        messageId: addressMessageId,
        coinType: fromAccountId?.coin
      })
    }, [
      showFilecoinFEVMWarning,
      fevmTranslatedAddresses,
      searchValue,
      addressMessageId,
      fromAccountId?.coin
    ])

    const showAddressMessage =
      (addressMessageInformation &&
        addressMessageInformation.id ===
          AddressMessageInfoIds.sameAddressError &&
        filteredAccounts.length !== 0) ||
      (!isSearchingForDomain &&
        addressMessageInformation &&
        filteredAccounts.length === 0 &&
        filteredAddressHistory.length === 0)

    // Methods
    const onSelectAccount = React.useCallback(
      async (account: BraveWallet.AccountInfo) => {
        if (
          account.accountId.coin === BraveWallet.CoinType.BTC ||
          account.accountId.coin === BraveWallet.CoinType.ZEC
        ) {
          const generatedAddress = await generateReceiveAddress(
            account.accountId
          ).unwrap()
          setToAddressOrUrl(generatedAddress)
        } else {
          setToAddressOrUrl(account.address)
        }
        setResolvedDomainAddress('')
        onClose()
      },
      [
        setToAddressOrUrl,
        generateReceiveAddress,
        onClose,
        setResolvedDomainAddress
      ]
    )

    const onSelectAddress = React.useCallback(
      (address: string) => {
        setToAddressOrUrl(address)
        setResolvedDomainAddress(resolvedDomainAddress)
        onClose()
      },
      [
        setToAddressOrUrl,
        setResolvedDomainAddress,
        resolvedDomainAddress,
        onClose
      ]
    )

    const onENSConsent = React.useCallback(() => {
      enableEnsOffchainLookup()
      dismissOffchainEnsWarning(true)
    }, [enableEnsOffchainLookup])

    const onClickDeleteAddress = React.useCallback(
      (address: string) => {
        if (addressHistoryIdentifier) {
          const newAddressHistoryForNetwork =
            addressHistoryByNetworkOrCoin.filter(
              (addr: string) => addr !== address
            )
          let history = addressHistory
          history[addressHistoryIdentifier] = newAddressHistoryForNetwork
          setAddressHistory(history)
        }
      },
      [
        addressHistory,
        addressHistoryByNetworkOrCoin,
        addressHistoryIdentifier,
        setAddressHistory
      ]
    )

    const onClickDisableAddressHistory = React.useCallback(() => {
      setAddressHistory({})
      setIsAddressHistoryDisabled(true)
    }, [setAddressHistory, setIsAddressHistoryDisabled])

    const onClickEnableAddressHistory = React.useCallback(() => {
      setIsAddressHistoryDisabled(false)
    }, [setIsAddressHistoryDisabled])

    if (showChecksumInfo) {
      return (
        <PopupModal
          onClose={onClose}
          onBack={() => setShowChecksumInfo(false)}
          title=''
          width='560px'
          height='90vh'
          ref={forwardedRef}
        >
          <ChecksumInfo />
        </PopupModal>
      )
    }

    // render
    return (
      <PopupModal
        onClose={onClose}
        title={getLocale('braveWalletChooseRecipient')}
        width='560px'
        height='90vh'
        ref={forwardedRef}
      >
        <Wrapper
          fullWidth={true}
          justifyContent='flex-start'
        >
          <SearchBoxContainer
            fullWidth={true}
            padding='0px 40px'
            margin='0px 0px 16px 0px'
          >
            <Row
              width='100%'
              justifyContent='flex-start'
              padding='0px 4px'
              marginBottom={4}
            >
              <LabelText
                textSize='12px'
                isBold={true}
              >
                {getLocale('braveWalletSendTo')}
              </LabelText>
            </Row>
            <AddressInput
              value={searchValue}
              onInput={(e) => setSearchValue(e.value)}
              placeholder={getLocale('braveWalletAddressOrDomainPlaceholder')}
              type='text'
              disabled={isGeneratingAddress}
            >
              <div slot='right-icon'>
                <Icon name='copy-plain-text' />
              </div>
            </AddressInput>
          </SearchBoxContainer>
          {isGeneratingAddress ? (
            <Column
              fullHeight
              fullWidth
            >
              <ProgressRing mode='indeterminate' />
            </Column>
          ) : (
            <ScrollContainer
              fullWidth={true}
              justifyContent='flex-start'
            >
              {!isAddressHistoryDisabled &&
                filteredAddressHistory.length !== 0 && (
                  <>
                    <Row
                      width='100%'
                      justifyContent='flex-start'
                      padding='0px 8px'
                      margin='4px 0px'
                    >
                      <LabelText
                        textSize='12px'
                        isBold={true}
                      >
                        {getLocale('braveWalletRecentAddresses')}
                      </LabelText>
                    </Row>
                    {filteredAddressHistory.map((address) => (
                      <Row
                        key={address}
                        gap='12px'
                      >
                        <AddressButton onClick={() => onSelectAddress(address)}>
                          <WalletIcon />
                          <Column alignItems='flext-start'>
                            <AddressButtonText
                              textSize='14px'
                              isBold={true}
                              textColor='primary'
                              textAlign='left'
                            >
                              {address}
                            </AddressButtonText>
                          </Column>
                        </AddressButton>
                        <Button
                          kind='plain-faint'
                          size='small'
                          onClick={() => onClickDeleteAddress(address)}
                        >
                          <TrashIcon />
                        </Button>
                      </Row>
                    ))}
                  </>
                )}
              {filteredAccounts.length !== 0 && (
                <>
                  <Row
                    width='100%'
                    justifyContent='flex-start'
                    padding='0px 8px'
                    margin={
                      filteredAddressHistory.length !== 0
                        ? '8px 0px 4px 0px'
                        : '0px 0px 4px 0px'
                    }
                  >
                    <LabelText
                      textSize='12px'
                      isBold={true}
                    >
                      {getLocale('braveWalletMyAddresses')}
                    </LabelText>
                  </Row>
                  {filteredAccounts.map((account) => (
                    <AccountListItem
                      key={account.accountId.uniqueKey}
                      account={account}
                      onClick={() => onSelectAccount(account)}
                      isSelected={
                        account.accountId.uniqueKey === fromAccountId?.uniqueKey
                      }
                      accountAlias={
                        fevmTranslatedAddresses?.[account.accountId.address]
                      }
                    />
                  ))}
                </>
              )}
              {isSearchingForDomain && (
                <Row margin='26px 0px 0px 0px'>
                  <DomainLoadIcon />
                  <Text
                    textSize='14px'
                    isBold={false}
                    textColor='secondary'
                  >
                    {getLocale('braveWalletSearchingForDomain')}
                  </Text>
                </Row>
              )}
              {filteredAccounts.length === 0 &&
                filteredAddressHistory.length === 0 &&
                !isSearchingForDomain &&
                !showEnsOffchainWarning &&
                addressMessageInformation?.type !== 'error' && (
                  <AddressButton onClick={() => onSelectAddress(searchValue)}>
                    <WalletIcon />
                    <Column alignItems='flext-start'>
                      <AddressButtonText
                        textSize='14px'
                        isBold={true}
                        textColor='primary'
                        textAlign='left'
                      >
                        {searchValue}
                      </AddressButtonText>
                      {searchValueHasValidExtension && (
                        <AddressButtonText
                          textSize='14px'
                          isBold={false}
                          textColor='secondary'
                          textAlign='left'
                        >
                          {resolvedDomainAddress}
                        </AddressButtonText>
                      )}
                    </Column>
                  </AddressButton>
                )}
              {showAddressMessage && (
                <>
                  <VerticalSpace space='8px' />
                  <AddressMessage
                    addressMessageInfo={addressMessageInformation}
                    onClickEnableENSOffchain={
                      addressMessageInformation.id ===
                      AddressMessageInfoIds.ensOffchainLookupWarning
                        ? onENSConsent
                        : undefined
                    }
                    onClickHowToSolve={
                      addressMessageInformation.id ===
                        AddressMessageInfoIds.invalidChecksumError ||
                      addressMessageInformation.id ===
                        AddressMessageInfoIds.missingChecksumWarning
                        ? () => setShowChecksumInfo(true)
                        : undefined
                    }
                  />
                </>
              )}
            </ScrollContainer>
          )}
          <Row
            gap='8px'
            padding={isPanel ? '16px' : '16px 16px 0px 16px'}
          >
            <Text
              textSize='12px'
              isBold={false}
              textColor='tertiary'
              textAlign='left'
            >
              {isAddressHistoryDisabled
                ? getLocale('braveWalletAddressHistoryDisabledDisclaimer')
                : getLocale('braveWalletAddressHistoryEnabledDisclaimer')}
            </Text>
            <div>
              <Button
                kind='plain'
                size='tiny'
                onClick={
                  isAddressHistoryDisabled
                    ? onClickEnableAddressHistory
                    : onClickDisableAddressHistory
                }
              >
                {isAddressHistoryDisabled
                  ? getLocale('braveWalletButtonEnable')
                  : getLocale('braveWalletDisableAddressHistory')}
              </Button>
            </div>
          </Row>
        </Wrapper>
      </PopupModal>
    )
  }
)

function getAddressMessageInfo({
  fevmTranslatedAddresses,
  showFilecoinFEVMWarning,
  toAddressOrUrl,
  messageId,
  coinType
}: {
  showFilecoinFEVMWarning: boolean
  fevmTranslatedAddresses:
    | Map<string, { address: string; fvmAddress: string }>
    | undefined
  toAddressOrUrl: string
  messageId: AddressMessageInfoIds | undefined
  coinType: BraveWallet.CoinType | undefined
}): AddressMessageInfo | undefined {
  if (showFilecoinFEVMWarning) {
    return {
      ...FEVMAddressConvertionMessage,
      placeholder: fevmTranslatedAddresses?.[toAddressOrUrl]
    }
  }
  if (
    messageId === AddressMessageInfoIds.invalidAddressError &&
    // Checking for not undefined here since BTC coinType is 0
    // which is a falsey value.
    coinType !== undefined
  ) {
    return {
      ...InvalidAddressMessage,
      placeholder: CoinTypesMap[coinType]
    }
  }
  if (
    messageId === AddressMessageInfoIds.hasNoDomainAddress &&
    // Checking for not undefined here since BTC coinType is 0
    // which is a falsey value.
    coinType !== undefined
  ) {
    return {
      ...HasNoDomainAddressMessage,
      placeholder: CoinTypesMap[coinType]
    }
  }

  return AddressValidationMessages.find((message) => message.id === messageId)
}

const validateETHAddress = (address: string, checksumAddress: string) => {
  if (!isValidEVMAddress(address)) {
    return AddressMessageInfoIds.invalidAddressError
  }

  if (checksumAddress === address) {
    return undefined
  }

  if ([address.toLowerCase(), address.toUpperCase()].includes(address)) {
    return AddressMessageInfoIds.missingChecksumWarning
  }

  return AddressMessageInfoIds.invalidChecksumError
}

const processEthereumAddress = (
  addressOrUrl: string,
  token: BraveWallet.BlockchainToken | undefined,
  checksumAddress: string
) => {
  if (
    token &&
    (token.chainId === BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID ||
      token.chainId === BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID) &&
    isValidFilAddress(addressOrUrl)
  ) {
    return undefined
  }

  return validateETHAddress(addressOrUrl, checksumAddress)
}

const processZCashAddress = (
  zecAddressValidationResult: BraveWallet.ZCashAddressValidationResult
) => {
  if (
    zecAddressValidationResult ===
    BraveWallet.ZCashAddressValidationResult.Unknown
  ) {
    return undefined
  }
  if (
    zecAddressValidationResult ===
    BraveWallet.ZCashAddressValidationResult.InvalidUnified
  ) {
    return AddressMessageInfoIds.invalidUnifiedAddressError
  }
  if (
    zecAddressValidationResult !==
    BraveWallet.ZCashAddressValidationResult.ValidTransparent &&
    zecAddressValidationResult !==
    BraveWallet.ZCashAddressValidationResult.ValidShielded
  ) {
    return AddressMessageInfoIds.invalidAddressError
  }
  return undefined
}

const processFilecoinAddress = (addressOrUrl: string, checksum: string) => {
  const valueToLowerCase = addressOrUrl.toLowerCase()

  // If value starts with 0x, will check if it's a valid address
  if (valueToLowerCase.startsWith('0x')) {
    return validateETHAddress(addressOrUrl, checksum)
  }

  if (!isValidFilAddress(valueToLowerCase)) {
    return AddressMessageInfoIds.invalidAddressError
  }

  // Default
  return undefined
}

const processSolanaAddress = (isBase58Encoded: boolean | undefined) => {
  // Check if value is a Base58 Encoded Solana Pubkey
  if (!isBase58Encoded) {
    return AddressMessageInfoIds.invalidAddressError
  }

  return undefined
}

const processBitcoinAddress = (addressOrUrl: string, testnet: boolean) => {
  if (!isValidBtcAddress(addressOrUrl, testnet)) {
    return AddressMessageInfoIds.invalidAddressError
  }

  return undefined
}

const processDomainLookupResponseWarning = (
  urlHasValidExtension: boolean,
  resolvedAddress: string | undefined,
  hasDomainLookupError: boolean,
  showEnsOffchainWarning: boolean,
  isOffChainEnsWarningDismissed: boolean,
  fromAccountAddress?: string
) => {
  if (showEnsOffchainWarning && !isOffChainEnsWarningDismissed) {
    return AddressMessageInfoIds.ensOffchainLookupWarning
  }

  if (!urlHasValidExtension) {
    return AddressMessageInfoIds.invalidDomainExtension
  }

  if (hasDomainLookupError || !resolvedAddress) {
    return AddressMessageInfoIds.hasNoDomainAddress
  }

  // If found address is the same as the selectedAccounts Wallet Address
  if (
    fromAccountAddress &&
    resolvedAddress.toLowerCase() === fromAccountAddress.toLowerCase()
  ) {
    return AddressMessageInfoIds.sameAddressError
  }

  return undefined
}

function processAddressOrUrl({
  addressOrUrl,
  fromAccountAddress,
  ethAddressChecksum,
  isBase58,
  coinType,
  zecAddressValidationResult,
  token,
  fullTokenList,
  isValidExtension,
  resolvedDomainAddress,
  isSearchingForDomain,
  hasNameServiceError,
  showEnsOffchainWarning,
  isOffChainEnsWarningDismissed
}: {
  addressOrUrl: string
  fromAccountAddress: string | undefined
  coinType: BraveWallet.CoinType | undefined
  token: BraveWallet.BlockchainToken | undefined
  ethAddressChecksum: string
  isBase58: boolean
  zecAddressValidationResult: BraveWallet.ZCashAddressValidationResult
  fullTokenList: BraveWallet.BlockchainToken[]
  isValidExtension: boolean
  resolvedDomainAddress: string | undefined
  isSearchingForDomain: boolean
  hasNameServiceError: boolean
  showEnsOffchainWarning: boolean
  isOffChainEnsWarningDismissed: boolean
}) {
  // Do nothing if value is an empty string
  if (addressOrUrl === '') {
    return undefined
  }

  if (addressOrUrl.includes('.') && !isSearchingForDomain) {
    return processDomainLookupResponseWarning(
      isValidExtension,
      resolvedDomainAddress,
      hasNameServiceError,
      showEnsOffchainWarning,
      isOffChainEnsWarningDismissed,
      fromAccountAddress
    )
  }

  if (
    fromAccountAddress &&
    fromAccountAddress.toLowerCase() === addressOrUrl.toLowerCase()
  ) {
    return AddressMessageInfoIds.sameAddressError
  }

  if (findTokenByContractAddress(addressOrUrl, fullTokenList) !== undefined) {
    return AddressMessageInfoIds.contractAddressError
  }

  switch (coinType) {
    case undefined:
      return undefined
    case BraveWallet.CoinType.ETH: {
      return processEthereumAddress(addressOrUrl, token, ethAddressChecksum)
    }
    case BraveWallet.CoinType.FIL: {
      return processFilecoinAddress(addressOrUrl, ethAddressChecksum)
    }
    case BraveWallet.CoinType.SOL: {
      return processSolanaAddress(isBase58)
    }
    case BraveWallet.CoinType.BTC: {
      return processBitcoinAddress(
        addressOrUrl,
        token?.chainId === BraveWallet.BITCOIN_TESTNET
      )
    }
    case BraveWallet.CoinType.ZEC: {
      return processZCashAddress(zecAddressValidationResult)
    }
    default: {
      console.log(`Unknown coin ${coinType}`)
      return undefined
    }
  }
}
