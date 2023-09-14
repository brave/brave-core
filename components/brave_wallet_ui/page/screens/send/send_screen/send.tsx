// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import {
  useParams,
  useHistory,
  useLocation
} from 'react-router'

// Messages
import { ENSOffchainLookupMessage, FEVMAddressConvertionMessage, FailedChecksumMessage } from '../send-ui-messages'

// Types
import {
  SendPageTabHashes,
  AddressMessageInfo,
  WalletRoutes
} from '../../../../constants/types'

// Selectors
import { WalletSelectors } from '../../../../common/selectors'
import { useUnsafeWalletSelector } from '../../../../common/hooks/use-safe-selector'

// Constants
import { allSupportedExtensions } from '../../../../common/constants/domain-extensions'

// Utils
import { getLocale } from '../../../../../common/locale'
import Amount from '../../../../utils/amount'
import { getBalance, formatTokenBalanceWithSymbol, getPercentAmount } from '../../../../utils/balance-utils'
import { computeFiatAmount } from '../../../../utils/pricing-utils'
import { endsWithAny } from '../../../../utils/string-utils'
import { getPriceIdForToken } from '../../../../utils/api-utils'

// Hooks
import { useSend } from '../../../../common/hooks/send'
import {
  useScopedBalanceUpdater
} from '../../../../common/hooks/use-scoped-balance-updater'
import { useOnClickOutside } from '../../../../common/hooks/useOnClickOutside'
import {
  useGetDefaultFiatCurrencyQuery,
  useSetSelectedAccountMutation,
  useSetNetworkMutation,
  useGetTokenSpotPricesQuery,
  useGetUserTokensRegistryQuery
} from '../../../../common/slices/api.slice'
import { useSelectedAccountQuery } from '../../../../common/slices/api.slice.extra'
import {
  querySubscriptionOptions60s
} from '../../../../common/slices/constants'
import {
  selectAllVisibleUserAssetsFromQueryResult
} from '../../../../common/slices/entities/blockchain-token.entity'

// Styled Components
import {
  SendContainer,
  SectionBox,
  AddressInput,
  AmountInput,
  DIVForWidth,
  InputRow,
  DomainLoadIcon,
  sendContainerWidth
} from './send.style'
import { Column, Text, Row, HorizontalDivider } from '../shared.styles'

// Components
import { SelectSendOptionButton } from '../components/select-send-option-button/select-send-option-button'
import { StandardButton } from '../components/standard-button/standard-button'
import { SelectTokenButton } from '../components/select-token-button/select-token-button'
import { PresetButton } from '../components/preset-button/preset-button'
import { AccountSelector } from '../components/account-selector/account-selector'
import { AddressMessage } from '../components/address-message/address-message'
import { SelectTokenModal } from '../components/select-token-modal/select-token-modal'
import { CopyAddress } from '../components/copy-address/copy-address'
import { ChecksumInfoModal } from '../components/checksum-info-modal/checksum-info-modal'
import WalletPageWrapper from '../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import { PageTitleHeader } from '../../../../components/desktop/card-headers/page-title-header'

interface Props {
  isAndroid?: boolean
}

export const Send = (props: Props) => {
  const { isAndroid } = props

  // Wallet Selectors
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)

  // routing
  const {
    chainId,
    accountAddress,
    contractAddressOrSymbol,
    tokenId
  } = useParams<{
    chainId?: string
    accountAddress?: string
    contractAddressOrSymbol?: string
    tokenId?: string
  }>()
  const { hash } = useLocation()
  const history = useHistory()

  const {
    toAddressOrUrl,
    toAddress,
    enableEnsOffchainLookup,
    showEnsOffchainWarning,
    setShowEnsOffchainWarning,
    showFilecoinFEVMWarning,
    fevmTranslatedAddresses,
    addressError,
    addressWarning,
    sendAmount,
    selectedSendAsset,
    sendAmountValidationError,
    setSendAmount,
    updateToAddressOrUrl,
    submitSend,
    selectSendAsset,
    searchingForDomain,
    processAddressOrUrl
  } = useSend()

  // Queries & Mutations
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const [setSelectedAccount] = useSetSelectedAccountMutation()
  const [setNetwork] = useSetNetworkMutation()
  const { data: selectedAccount } = useSelectedAccountQuery()

  // Refs
  const checksumInfoModalRef = React.useRef<HTMLDivElement>(null)

  // State
  const [domainPosition, setDomainPosition] = React.useState<number>(0)
  const [showChecksumInfoModal, setShowChecksumInfoModal] = React.useState<boolean>(false)
  const [showSelectTokenModal, setShowSelectTokenModal] =
    React.useState<boolean>(false)

  // Refs
  const selectTokenModalRef = React.useRef<HTMLDivElement>(null)

  // Hooks
  useOnClickOutside(
    selectTokenModalRef,
    () => setShowSelectTokenModal(false),
    showSelectTokenModal
  )

  // Constants
  const selectedSendOption = hash
    ? hash as SendPageTabHashes
    : '#token' as SendPageTabHashes

  useOnClickOutside(
    checksumInfoModalRef,
    () => setShowChecksumInfoModal(false),
    showChecksumInfoModal
  )

  // Methods
  const handleInputAmountChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSendAmount(event.target.value)
    },
    []
  )

  const handleInputAddressChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      updateToAddressOrUrl(event.target.value)
    },
    [updateToAddressOrUrl]
  )

  const onSelectSendOption = React.useCallback(
    (option: SendPageTabHashes) => {
      selectSendAsset(undefined)
      history.push(`${WalletRoutes.SendPageStart}${option}`)
    }, [selectSendAsset])

  const onClickReviewOrENSConsent = React.useCallback(() => {
    if (showEnsOffchainWarning) {
      enableEnsOffchainLookup()
      setShowEnsOffchainWarning(false)
      processAddressOrUrl(toAddressOrUrl)
      return
    }
    submitSend()
  }, [
    showEnsOffchainWarning,
    setShowEnsOffchainWarning,
    submitSend,
    enableEnsOffchainLookup,
    processAddressOrUrl,
    toAddressOrUrl
  ])

  const updateLoadingIconPosition = React.useCallback((ref: HTMLDivElement | null) => {
    const position = ref?.clientWidth
    setDomainPosition(position ? position + 22 : 0)
  }, [])

  const {
    data: tokenBalancesRegistry,
    isLoading: isLoadingBalances,
  } = useScopedBalanceUpdater(
    selectedAccount && selectedSendAsset
      ? {
          network: {
            chainId: selectedSendAsset.chainId,
            coin: selectedAccount.accountId.coin
          },
          accounts: [selectedAccount],
          tokens: [selectedSendAsset]
        }
      : skipToken
    )

  const setPresetAmountValue = React.useCallback((percent: number) => {
    if (!selectedSendAsset || !selectedAccount) {
      return
    }

    setSendAmount(
      getPercentAmount(
        selectedSendAsset,
        selectedAccount.accountId,
        percent,
        tokenBalancesRegistry
      )
    )
  }, [setSendAmount, selectedSendAsset, selectedAccount, tokenBalancesRegistry])

  const sendAssetBalance = React.useMemo(() => {
    if (!selectedAccount || !selectedSendAsset || !tokenBalancesRegistry) {
      return ''
    }

    return getBalance(
      selectedAccount.accountId,
      selectedSendAsset,
      tokenBalancesRegistry
    )
  }, [selectedAccount, selectedSendAsset, tokenBalancesRegistry])

  const accountNameAndBalance = React.useMemo(() => {
    if (!selectedSendAsset || sendAssetBalance === '') {
      return ''
    }
    if (selectedSendOption === SendPageTabHashes.nft) {
      return selectedAccount?.name
    }
    return `${selectedAccount?.name}: ${formatTokenBalanceWithSymbol(
      sendAssetBalance,
      selectedSendAsset.decimals,
      selectedSendAsset.symbol,
      4
    )}`
  }, [
    selectedAccount?.name,
    selectedSendAsset,
    sendAssetBalance
  ])

  const insufficientFundsError = React.useMemo((): boolean => {
    if (!selectedSendAsset) {
      return false
    }

    const amountWei = new Amount(sendAmount).multiplyByDecimals(
      selectedSendAsset.decimals
    )

    if (amountWei.isZero()) {
      return false
    }

    return amountWei.gt(sendAssetBalance)
  }, [sendAssetBalance, sendAmount, selectedSendAsset])

  const tokenPriceIds = React.useMemo(() =>
    selectedSendAsset
      ? [getPriceIdForToken(selectedSendAsset)]
      : [],
    [selectedSendAsset]
  )

  const {
    data: spotPriceRegistry
  } = useGetTokenSpotPricesQuery(
    !isLoadingBalances && tokenPriceIds.length && defaultFiatCurrency
      ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s
  )

  const sendAmountFiatValue = React.useMemo(() => {
    if (
      !selectedSendAsset ||
      sendAssetBalance === '' ||
      selectedSendOption === SendPageTabHashes.nft
    ) {
      return ''
    }

    return computeFiatAmount({
      spotPriceRegistry,
      value: new Amount(sendAmount !== '' ? sendAmount : '0')
        .multiplyByDecimals(selectedSendAsset.decimals) // ETH → Wei conversion
        .toHex(),
      token: selectedSendAsset,
    }).formatAsFiat(defaultFiatCurrency)
  }, [
    spotPriceRegistry,
    selectedSendAsset,
    sendAmount,
    defaultFiatCurrency,
    sendAssetBalance,
    selectedSendOption
  ])

  const reviewButtonText = React.useMemo(() => {
    return showEnsOffchainWarning
      ? getLocale('braveWalletEnsOffChainButton')
      : searchingForDomain
        ? getLocale('braveWalletSearchingForDomain')
        : sendAmountValidationError
          ? getLocale('braveWalletDecimalPlacesError')
          : insufficientFundsError
            ? getLocale('braveWalletNotEnoughFunds')
            : (
              addressError !== undefined &&
              addressError !== '' &&
              addressError !== getLocale('braveWalletNotValidChecksumAddressError')
            )
              ? addressError
              : (
                addressWarning !== undefined &&
                addressWarning !== '' &&
                addressWarning !== getLocale('braveWalletAddressMissingChecksumInfoWarning')
              )
                ? addressWarning
                : getLocale('braveWalletReviewSend')
  }, [insufficientFundsError, addressError, addressWarning, sendAmountValidationError, searchingForDomain, showEnsOffchainWarning])

  const isReviewButtonDisabled = React.useMemo(() => {
    // We only need to check if showEnsOffchainWarning is true here to return
    // false early before any other checks are made. This is to allow the button
    // to be pressed to enable offchain lookup.
    return !showEnsOffchainWarning &&
      (searchingForDomain ||
        toAddressOrUrl === '' ||
        parseFloat(sendAmount) === 0 ||
        sendAmount === '' ||
        insufficientFundsError ||
        (addressError !== undefined && addressError !== '') ||
        sendAmountValidationError !== undefined)
  },
    [
      toAddressOrUrl,
      sendAmount,
      insufficientFundsError,
      addressError,
      sendAmountValidationError,
      searchingForDomain,
      showEnsOffchainWarning
    ]
  )

  const reviewButtonHasError = React.useMemo(() => {
    return searchingForDomain
      ? false
      : insufficientFundsError ||
      (addressError !== undefined &&
        addressError !== '' &&
        addressError !== getLocale('braveWalletNotValidChecksumAddressError'))
  }, [searchingForDomain, insufficientFundsError, addressError])

  const hasAddressError = React.useMemo(() => {
    return searchingForDomain
      ? false
      : !!addressError
  }, [searchingForDomain, addressError])

  const addressMessageInformation: AddressMessageInfo | undefined = React.useMemo(() => {
    if (showFilecoinFEVMWarning) {
      return {
        ...FEVMAddressConvertionMessage,
        placeholder: fevmTranslatedAddresses?.[toAddressOrUrl]
      }
    }
    if (showEnsOffchainWarning) {
      return ENSOffchainLookupMessage
    }
    if (addressError === getLocale('braveWalletNotValidChecksumAddressError')) {
      return { ...FailedChecksumMessage, type: 'error' }
    }
    if (addressWarning === getLocale('braveWalletAddressMissingChecksumInfoWarning')) {
      return { ...FailedChecksumMessage, type: 'warning' }
    }
    return undefined
  }, [toAddressOrUrl, showFilecoinFEVMWarning, fevmTranslatedAddresses,
      showEnsOffchainWarning, addressError, addressWarning])

  const showResolvedDomain = React.useMemo(() => {
    return (addressError === undefined ||
      addressError === '' ||
      addressError === getLocale('braveWalletSameAddressError')) &&
      toAddress &&
      endsWithAny(allSupportedExtensions, toAddressOrUrl.toLowerCase())
  }, [addressError, toAddress, toAddressOrUrl])

  const showSearchingForDomainIcon = React.useMemo(() => {
    return (endsWithAny(allSupportedExtensions, toAddressOrUrl.toLowerCase()) && searchingForDomain) ||
      showEnsOffchainWarning
  }, [toAddressOrUrl, searchingForDomain, showEnsOffchainWarning])

  const { userVisibleTokensInfo } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: result => ({
      userVisibleTokensInfo: selectAllVisibleUserAssetsFromQueryResult(result)
    })
  })

  const selectedAssetFromParams = React.useMemo(() => {
    if (!contractAddressOrSymbol || !chainId) return

    return userVisibleTokensInfo.find(token =>
      tokenId
        ? token.chainId === chainId &&
        token.contractAddress.toLowerCase() ===
        contractAddressOrSymbol.toLowerCase() &&
        token.tokenId === tokenId
        : (
          token.contractAddress.toLowerCase() ===
          contractAddressOrSymbol.toLowerCase() &&
          token.chainId === chainId) ||
        (
          token.symbol.toLowerCase() ===
          contractAddressOrSymbol.toLowerCase() &&
          token.chainId === chainId &&
          token.contractAddress === ''
        )
    )
  }, [
    userVisibleTokensInfo,
    chainId,
    contractAddressOrSymbol,
    tokenId
  ])

  const accountFromParams = React.useMemo(() => {
    return accounts.find(
      (account) => account.address === accountAddress
    )
  }, [accountAddress, accounts])

  // Effects
  React.useEffect(() => {
    // check if the user has selected an asset
    if (!chainId || !selectedAssetFromParams || !accountFromParams || selectedSendAsset) return

    ;(async () => {
      try {
        await setSelectedAccount(accountFromParams.accountId)
        await setNetwork({
          chainId: chainId,
          coin: selectedAssetFromParams.coin
        })
      } catch (e) {
        console.error(e)
      }
    })()

    selectSendAsset(selectedAssetFromParams)
  }, [
    selectSendAsset,
    selectedSendAsset,
    chainId,
    selectedAssetFromParams,
    accountFromParams
  ])

  // render
  return (
    <>
      <WalletPageWrapper
        wrapContentInBox={true}
        cardWidth={sendContainerWidth}
        noMinCardHeight={true}
        hideNav={isAndroid}
        hideHeader={isAndroid}
        cardHeader={
          <PageTitleHeader title={getLocale('braveWalletSend')}/>
        }
      >
        <SendContainer>
          <Row rowWidth='full' marginBottom={16}>
            <SelectSendOptionButton
              selectedSendOption={selectedSendOption}
              onClick={onSelectSendOption}
            />
          </Row>
          <SectionBox
            minHeight={150}
            hasError={insufficientFundsError}
          >
            {selectedSendOption === SendPageTabHashes.token &&
              <Column
                columnHeight='full'
                columnWidth='full'
                verticalAlign='space-between'
                horizontalAlign='space-between'
              >
                <Row
                  rowWidth='full'
                  horizontalAlign='flex-end'>
                  <Text textSize='14px' textColor='text03' maintainHeight={true} isBold={true}>
                    {accountNameAndBalance}
                  </Text>
                </Row>
                <Row
                  rowWidth='full'
                >
                  <Row>
                    <SelectTokenButton
                      onClick={() => setShowSelectTokenModal(true)}
                      token={selectedSendAsset}
                      selectedSendOption={selectedSendOption} />
                    {
                      selectedSendOption === SendPageTabHashes.token &&
                      selectedSendAsset &&
                      <>
                        <HorizontalDivider
                          height={28}
                          marginLeft={8}
                          marginRight={8}
                          dividerTheme='lighter'
                        />
                        <PresetButton buttonText={getLocale('braveWalletSendHalf')} onClick={() => setPresetAmountValue(0.5)} />
                        <PresetButton buttonText={getLocale('braveWalletSendMax')} onClick={() => setPresetAmountValue(1)} />
                      </>
                    }
                  </Row>
                  {selectedSendOption === SendPageTabHashes.token &&
                    <AmountInput
                      placeholder='0.0'
                      hasError={insufficientFundsError}
                      value={sendAmount}
                      onChange={handleInputAmountChange}
                    />
                  }
                </Row>
                <Row
                  rowWidth='full'
                  horizontalAlign='flex-end'>
                  <Text textSize='14px' textColor='text03' maintainHeight={true} isBold={false}>
                    {sendAmountFiatValue}
                  </Text>
                </Row>
              </Column>
            }
            {selectedSendOption === SendPageTabHashes.nft &&
              <Row
                rowWidth='full'
                rowHeight='full'
              >
                <Column
                  columnHeight='full'
                  verticalAlign='center'
                >
                  <SelectTokenButton
                    onClick={() => setShowSelectTokenModal(true)}
                    token={selectedSendAsset}
                    selectedSendOption={selectedSendOption} />
                </Column>
                <Column
                  columnHeight='full'
                  verticalAlign='flex-start'
                  horizontalAlign='flex-end'
                >
                  <Text textSize='14px' textColor='text03' maintainHeight={true} isBold={true} textAlign='right'>
                    {accountNameAndBalance}
                  </Text>
                </Column>
              </Row>
            }
          </SectionBox>
          <SectionBox
            hasError={hasAddressError}
            hasWarning={addressWarning !== undefined && addressWarning !== ''}
            noPadding={true}
          >
            <InputRow
              rowWidth='full'
              verticalAlign='center'
              paddingTop={16}
              paddingBottom={showResolvedDomain ? 4 : 16}
              horizontalPadding={16}
            >
              {showSearchingForDomainIcon &&
                <DomainLoadIcon position={domainPosition} />
              }
              <DIVForWidth ref={(ref) => updateLoadingIconPosition(ref)}>{toAddressOrUrl}</DIVForWidth>
              <AddressInput
                placeholder={getLocale('braveWalletEnterRecipientAddress')}
                hasError={hasAddressError}
                value={toAddressOrUrl}
                onChange={handleInputAddressChange}
                spellCheck={false}
                disabled={!selectedSendAsset}
              />
              <AccountSelector asset={selectedSendAsset} disabled={!selectedSendAsset} onSelectAddress={updateToAddressOrUrl} />
            </InputRow>
            {showResolvedDomain &&
              <CopyAddress address={toAddress} />
            }
            {addressMessageInformation &&
              <AddressMessage
                addressMessageInfo={addressMessageInformation}
                onClickHowToSolve={
                  (addressError === getLocale('braveWalletNotValidChecksumAddressError') ||
                    addressWarning === getLocale('braveWalletAddressMissingChecksumInfoWarning'))
                    ? () => setShowChecksumInfoModal(true)
                    : undefined
                }
              />
            }
          </SectionBox>
          <StandardButton
            buttonText={reviewButtonText}
            onClick={onClickReviewOrENSConsent}
            buttonType='primary'
            buttonWidth='full'
            isLoading={searchingForDomain}
            disabled={isReviewButtonDisabled}
            hasError={reviewButtonHasError}
          />
        </SendContainer>
      </WalletPageWrapper>
      {showSelectTokenModal &&
        <SelectTokenModal
          onClose={() => setShowSelectTokenModal(false)}
          selectedSendOption={selectedSendOption}
          ref={selectTokenModalRef}
          selectSendAsset={selectSendAsset}
        />
      }
      {showChecksumInfoModal &&
        <ChecksumInfoModal
          onClose={() => setShowChecksumInfoModal(false)}
          ref={checksumInfoModalRef}
        />
      }
    </>
  )
}

export default Send
