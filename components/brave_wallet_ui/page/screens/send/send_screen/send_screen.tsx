// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/key-spacing */
import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { useParams, useHistory, useLocation } from 'react-router'

// Messages
import {
  ENSOffchainLookupMessage,
  FEVMAddressConvertionMessage,
  FailedChecksumMessage
} from '../send-ui-messages'

// Types
import {
  SendPageTabHashes,
  AddressMessageInfo,
  WalletRoutes,
  CoinTypesMap,
  BraveWallet,
  BaseTransactionParams,
  AmountValidationErrorType
} from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'
import Amount from '../../../../utils/amount'
import {
  getBalance,
  formatTokenBalanceWithSymbol,
  getPercentAmount
} from '../../../../utils/balance-utils'
import { computeFiatAmount } from '../../../../utils/pricing-utils'
import {
  findTokenByContractAddress,
  getAssetIdKey
} from '../../../../utils/asset-utils'
import { endsWithAny } from '../../../../utils/string-utils'
import {
  supportedENSExtensions,
  supportedSNSExtensions,
  supportedUDExtensions
} from '../../../../common/constants/domain-extensions'
import { getPriceIdForToken } from '../../../../utils/api-utils'
import {
  isValidEVMAddress,
  isValidFilAddress
} from '../../../../utils/address-utils'
import {
  selectAllVisibleUserAssetsFromQueryResult //
} from '../../../../common/slices/entities/blockchain-token.entity'

// Hooks
import {
  useScopedBalanceUpdater
} from '../../../../common/hooks/use-scoped-balance-updater'
import { useModal } from '../../../../common/hooks/useOnClickOutside'
import {
  useGetDefaultFiatCurrencyQuery,
  useSetSelectedAccountMutation,
  useSetNetworkMutation,
  useGetTokenSpotPricesQuery,
  useGetUserTokensRegistryQuery,
  useGetSelectedChainQuery,
  useEnableEnsOffchainLookupMutation,
  useGetFVMAddressQuery,
  useGetEthAddressChecksumQuery,
  useGetIsBase58EncodedSolPubkeyQuery,
  useSendSPLTransferMutation,
  useSendTransactionMutation,
  useSendERC20TransferMutation,
  useSendERC721TransferFromMutation,
  useSendETHFilForwarderTransferMutation,
  useGetAddressFromNameServiceUrlQuery,
} from '../../../../common/slices/api.slice'
import {
  useAccountFromAddressQuery,
  useGetCombinedTokensListQuery,
  useSelectedAccountQuery
} from '../../../../common/slices/api.slice.extra'
import {
  querySubscriptionOptions60s //
} from '../../../../common/slices/constants'

// Styled Components
import {
  SendContainer,
  SectionBox,
  AddressInput,
  AmountInput,
  DIVForWidth,
  InputRow,
  sendContainerWidth,
  SmallLoadingRing,
  DomainLoadIcon
} from './send.style'
import { Column, Text, Row, HorizontalDivider } from '../shared.styles'

// Components
import {
  SelectSendOptionButton //
} from '../components/select-send-option-button/select-send-option-button'
import { StandardButton } from '../components/standard-button/standard-button'
import {
  SelectTokenButton //
} from '../components/select-token-button/select-token-button'
import { PresetButton } from '../components/preset-button/preset-button'
import {
  AccountSelector //
} from '../components/account-selector/account-selector'
import { AddressMessage } from '../components/address-message/address-message'
import {
  SelectTokenModal //
} from '../components/select-token-modal/select-token-modal'
import { CopyAddress } from '../components/copy-address/copy-address'
import {
  ChecksumInfoModal //
} from '../components/checksum-info-modal/checksum-info-modal'
import {
  WalletPageWrapper //
} from '../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import {
  PageTitleHeader //
} from '../../../../components/desktop/card-headers/page-title-header'

interface Props {
  isAndroid?: boolean
}

const ErrorFailedChecksumMessage: AddressMessageInfo = {
  ...FailedChecksumMessage,
  type: 'error'
}

const WarningFailedChecksumMessage: AddressMessageInfo = {
  ...FailedChecksumMessage,
  type: 'warning'
}

export const SendScreen = React.memo((props: Props) => {
  const { isAndroid = false } = props

  // routing
  const { chainId, accountAddress, contractAddressOrSymbol, tokenId } =
    useParams<{
      chainId?: string
      accountAddress?: string
      contractAddressOrSymbol?: string
      tokenId?: string
    }>()
  const history = useHistory()
  const { hash } = useLocation()
  const selectedSendOption = (hash as SendPageTabHashes) || '#token'

  const { account: accountFromParams } =
    useAccountFromAddressQuery(accountAddress)

  // Refs
  const addressWidthRef = React.useRef<HTMLDivElement>(null)

  // State
  const [sendAmount, setSendAmount] = React.useState<string>('')
  const [selectedSendAsset, setSelectedSendAsset] = React.useState<
    BraveWallet.BlockchainToken | undefined
  >(undefined)
  const [toAddressOrUrl, setToAddressOrUrl] = React.useState<string>('')
  const trimmedToAddressOrUrl = toAddressOrUrl.trim()

  const [isOffChainEnsWarningDismissed, dismissOffchainEnsWarning] =
    React.useState<boolean>(false)

  const [domainPosition, setDomainPosition] = React.useState<number>(0)

  // Mutations
  const [enableEnsOffchainLookup] = useEnableEnsOffchainLookupMutation()
  const [setNetwork] = useSetNetworkMutation()
  const [setSelectedAccount] = useSetSelectedAccountMutation()
  const [sendSPLTransfer] = useSendSPLTransferMutation()
  const [sendTransaction] = useSendTransactionMutation()
  const [sendERC20Transfer] = useSendERC20TransferMutation()
  const [sendERC721TransferFrom] = useSendERC721TransferFromMutation()
  const [sendETHFilForwarderTransfer] = useSendETHFilForwarderTransferMutation()

  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: selectedAccount, isLoading: isLoadingSelectedAccount } =
    useSelectedAccountQuery()

  const { data: fullTokenList } = useGetCombinedTokensListQuery()

  const { userVisibleTokensInfo } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: result => ({
      userVisibleTokensInfo: selectAllVisibleUserAssetsFromQueryResult(result)
    })
  })

  const selectedAssetFromParams = React.useMemo(() => {
    if (!contractAddressOrSymbol || !chainId) return

    const contractOrSymbolLower = contractAddressOrSymbol.toLowerCase()

    return userVisibleTokensInfo.find((token) =>
      tokenId
        ? token.chainId === chainId &&
          token.contractAddress.toLowerCase() === contractOrSymbolLower &&
          token.tokenId === tokenId
        : (token.contractAddress.toLowerCase() === contractOrSymbolLower &&
            token.chainId === chainId) ||
          (token.symbol.toLowerCase() === contractOrSymbolLower &&
            token.chainId === chainId &&
            token.contractAddress === '')
    )
  }, [userVisibleTokensInfo, chainId, contractAddressOrSymbol, tokenId])

  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const { data: tokenBalancesRegistry, isFetching: isLoadingBalances } =
    useScopedBalanceUpdater(
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

  const { data: spotPriceRegistry, isFetching: isLoadingSpotPrices } =
    useGetTokenSpotPricesQuery(
      !isLoadingBalances && selectedSendAsset && defaultFiatCurrency
        ? {
            ids: [getPriceIdForToken(selectedSendAsset)],
            toCurrency: defaultFiatCurrency
          }
        : skipToken,
      querySubscriptionOptions60s
    )

  // Domain name lookup Queries
  const selectedSendAssetId = selectedSendAsset
    ? getAssetIdKey(selectedSendAsset)
    : null

  const lowerCaseToAddress = toAddressOrUrl.toLowerCase()

  const toAddressHasValidExtension = toAddressOrUrl
    ? endsWithAny(supportedUDExtensions, lowerCaseToAddress) ||
      (selectedSendAsset?.coin === BraveWallet.CoinType.SOL &&
        endsWithAny(supportedSNSExtensions, lowerCaseToAddress)) ||
      (selectedSendAsset?.coin === BraveWallet.CoinType.ETH &&
        endsWithAny(supportedENSExtensions, lowerCaseToAddress))
    : false

  const {
    data: nameServiceInfo,
    isFetching: isSearchingForDomain,
    isError: hasNameServiceError = false
  } = useGetAddressFromNameServiceUrlQuery(
    toAddressHasValidExtension
      ? {
          tokenId: selectedSendAssetId,
          url: toAddressOrUrl
        }
      : skipToken
  )

  const resolvedDomainAddress = nameServiceInfo?.address || ''
  const showEnsOffchainWarning =
    nameServiceInfo?.requireOffchainConsent || false

  const { data: fevmTranslatedAddresses } = useGetFVMAddressQuery(
    selectedSendAsset?.coin === BraveWallet.CoinType.FIL &&
      trimmedToAddressOrUrl
      ? {
          coin: selectedSendAsset.coin,
          addresses: [trimmedToAddressOrUrl],
          isMainNet: selectedSendAsset.chainId === BraveWallet.FILECOIN_MAINNET
        }
      : skipToken
  )

  const { data: isBase58 = false } = useGetIsBase58EncodedSolPubkeyQuery(
    !toAddressHasValidExtension &&
      selectedAccount?.accountId.coin === BraveWallet.CoinType.SOL &&
      trimmedToAddressOrUrl
      ? trimmedToAddressOrUrl
      : skipToken
  )

  const isValidEvmAddress = isValidEVMAddress(trimmedToAddressOrUrl)

  const { data: ethAddressChecksum = '' } = useGetEthAddressChecksumQuery(
    isValidEvmAddress ? trimmedToAddressOrUrl : skipToken
  )

  // memos & computed
  const sendAmountValidationError: AmountValidationErrorType | undefined =
    React.useMemo(() => {
      if (!sendAmount || !selectedSendAsset) {
        return
      }

      // extract BigNumber object wrapped by Amount
      const amountBN = ethToWeiAmount(sendAmount, selectedSendAsset).value

      const amountDP = amountBN && amountBN.decimalPlaces()
      return amountDP && amountDP > 0 ? 'fromAmountDecimalsOverflow' : undefined
    }, [sendAmount, selectedSendAsset])

  const sendAssetBalance =
    !selectedAccount || !selectedSendAsset || !tokenBalancesRegistry
      ? ''
      : getBalance(
          selectedAccount.accountId,
          selectedSendAsset,
          tokenBalancesRegistry
        )

  const accountNameAndBalance =
    !selectedSendAsset || sendAssetBalance === ''
      ? ''
      : selectedSendOption === SendPageTabHashes.nft
      ? selectedAccount?.name
      : `${selectedAccount?.name}: ${formatTokenBalanceWithSymbol(
          sendAssetBalance,
          selectedSendAsset.decimals,
          selectedSendAsset.symbol,
          4
        )}`

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
      value: ethToWeiAmount(
        sendAmount !== '' ? sendAmount : '0',
        selectedSendAsset
      ).toHex(),
      token: selectedSendAsset
    }).formatAsFiat(defaultFiatCurrency)
  }, [
    spotPriceRegistry,
    selectedSendAsset,
    sendAmount,
    defaultFiatCurrency,
    sendAssetBalance,
    selectedSendOption
  ])

  const doneSearchingForDomain = !isSearchingForDomain
  const hasResolvedDomain = doneSearchingForDomain && resolvedDomainAddress
  const hasValidResolvedDomain = toAddressHasValidExtension && hasResolvedDomain

  const domainErrorLocaleKey =
    toAddressOrUrl && doneSearchingForDomain
      ? processDomainLookupResponseWarning(
          toAddressHasValidExtension,
          resolvedDomainAddress,
          hasNameServiceError,
          showEnsOffchainWarning,
          selectedAccount?.address
        )
      : undefined

  const resolvedDomainOrToAddressOrUrl = hasValidResolvedDomain
    ? resolvedDomainAddress
    : trimmedToAddressOrUrl

  const toAddressIsTokenContract = resolvedDomainOrToAddressOrUrl
    ? findTokenByContractAddress(
        resolvedDomainOrToAddressOrUrl,
        fullTokenList
      ) !== undefined
    : undefined

  const toAddressIsSelectedAccount =
    selectedAccount &&
    resolvedDomainOrToAddressOrUrl.toLowerCase() ===
      selectedAccount.address.toLowerCase()

  const addressWarningLocaleKey = toAddressIsTokenContract
    ? 'braveWalletContractAddressError'
    : isValidEvmAddress &&
      ethAddressChecksum !== toAddressOrUrl &&
      [lowerCaseToAddress, toAddressOrUrl.toUpperCase()].includes(
        toAddressOrUrl
      )
    ? 'braveWalletAddressMissingChecksumInfoWarning'
    : undefined

  const hasAddressWarning = Boolean(addressWarningLocaleKey)

  const addressErrorLocaleKey = toAddressIsSelectedAccount
    ? 'braveWalletSameAddressError'
    : trimmedToAddressOrUrl.includes('.')
    ? domainErrorLocaleKey
    : selectedAccount
    ? addressWarningLocaleKey !==
        'braveWalletAddressMissingChecksumInfoWarning' ?
      processAddressOrUrl({
        addressOrUrl: trimmedToAddressOrUrl,
        ethAddressChecksum,
        isBase58,
        coinType: selectedAccount.accountId.coin ?? BraveWallet.CoinType.ETH,
        selectedSendAsset
      }) : undefined
    : undefined

  const addressError = addressErrorLocaleKey
    ? getLocale(addressErrorLocaleKey).replace(
        '$1',
        CoinTypesMap[selectedNetwork?.coin ?? 0]
      )
    : undefined

  const hasAddressError = doneSearchingForDomain && Boolean(addressError)

  const showResolvedDomain = hasValidResolvedDomain && !hasAddressError

  // reused locales
  const braveWalletAddressMissingChecksumInfoWarning = getLocale(
    'braveWalletAddressMissingChecksumInfoWarning'
  )
  const braveWalletNotValidChecksumAddressError = getLocale(
    'braveWalletNotValidChecksumAddressError'
  )

  const reviewButtonHasError =
    doneSearchingForDomain &&
    (insufficientFundsError ||
      (addressError !== undefined &&
        addressError !== '' &&
        addressError !== braveWalletNotValidChecksumAddressError))

  const showFilecoinFEVMWarning =
    selectedAccount?.accountId.coin === BraveWallet.CoinType.FIL
      ? trimmedToAddressOrUrl.startsWith('0x') &&
        !validateETHAddress(trimmedToAddressOrUrl, ethAddressChecksum)
      : false

  const addressMessageInformation: AddressMessageInfo | undefined =
    React.useMemo(
      getAddressMessageInfo({
        showFilecoinFEVMWarning,
        fevmTranslatedAddresses,
        toAddressOrUrl,
        showEnsOffchainWarning,
        addressErrorKey: addressErrorLocaleKey,
        addressWarningKey: addressWarningLocaleKey,
      }),
      [
        showFilecoinFEVMWarning,
        fevmTranslatedAddresses,
        toAddressOrUrl,
        showEnsOffchainWarning,
        addressErrorLocaleKey,
        addressWarningLocaleKey
      ]
    )

  // Methods
  const selectSendAsset = React.useCallback(
    (asset: BraveWallet.BlockchainToken | undefined) => {
      if (asset?.isErc721 || asset?.isNft) {
        setSendAmount('1')
      } else {
        setSendAmount('')
      }
      setToAddressOrUrl('')
      setSelectedSendAsset(asset)
    },
    []
  )

  const resetSendFields = React.useCallback(() => {
    selectSendAsset(undefined)
    setToAddressOrUrl('')
    setSendAmount('')
  }, [selectSendAsset])

  const submitSend = React.useCallback(async () => {
    if (!selectedSendAsset) {
      console.log('Failed to submit Send transaction: no send asset selected')
      return
    }

    if (!selectedAccount) {
      console.log('Failed to submit Send transaction: no account selected')
      return
    }

    if (!selectedNetwork) {
      console.log('Failed to submit Send transaction: no network selected')
      return
    }

    const fromAccount: BaseTransactionParams['fromAccount'] = {
      accountId: selectedAccount.accountId,
      address: selectedAccount.address,
      hardware: selectedAccount.hardware
    }

    const toAddress = showResolvedDomain
      ? resolvedDomainAddress
      : toAddressOrUrl

    selectedSendAsset.isErc20 &&
      (await sendERC20Transfer({
        network: selectedNetwork,
        fromAccount,
        to: toAddress,
        value: ethToWeiAmount(sendAmount, selectedSendAsset).toHex(),
        contractAddress: selectedSendAsset.contractAddress
      }))

    selectedSendAsset.isErc721 &&
      (await sendERC721TransferFrom({
        network: selectedNetwork,
        fromAccount,
        to: toAddress,
        value: '',
        contractAddress: selectedSendAsset.contractAddress,
        tokenId: selectedSendAsset.tokenId ?? ''
      }))

    if (
      selectedAccount.accountId.coin === BraveWallet.CoinType.SOL &&
      selectedSendAsset.contractAddress !== '' &&
      !selectedSendAsset.isErc20 &&
      !selectedSendAsset.isErc721
    ) {
      await sendSPLTransfer({
        network: selectedNetwork,
        fromAccount,
        to: toAddress,
        value: !selectedSendAsset.isNft
          ? new Amount(sendAmount)
              .multiplyByDecimals(selectedSendAsset.decimals)
              .toHex()
          : new Amount(sendAmount).toHex(),
        splTokenMintAddress: selectedSendAsset.contractAddress
      })
      resetSendFields()
      return
    }

    if (selectedAccount.accountId.coin === BraveWallet.CoinType.FIL) {
      await sendTransaction({
        network: selectedNetwork,
        fromAccount,
        to: toAddress,
        value: new Amount(sendAmount)
          .multiplyByDecimals(selectedSendAsset.decimals)
          .toNumber()
          .toString()
      })
      resetSendFields()
      return
    }

    if (selectedSendAsset.isErc721 || selectedSendAsset.isErc20) {
      resetSendFields()
      return
    }

    if (
      selectedAccount.accountId.coin === BraveWallet.CoinType.ETH &&
      (selectedSendAsset.chainId ===
        BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID ||
        selectedSendAsset.chainId ===
          BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID) &&
      isValidFilAddress(toAddress)
    ) {
      await sendETHFilForwarderTransfer({
        network: selectedNetwork,
        fromAccount,
        to: toAddress,
        value: ethToWeiAmount(sendAmount, selectedSendAsset).toHex(),
        contractAddress: '0x2b3ef6906429b580b7b2080de5ca893bc282c225'
      })
      resetSendFields()
      return
    }

    await sendTransaction({
      network: selectedNetwork,
      fromAccount,
      to: toAddress,
      value:
        selectedAccount.accountId.coin === BraveWallet.CoinType.FIL
          ? new Amount(sendAmount)
              .multiplyByDecimals(selectedSendAsset.decimals)
              .toString()
          : new Amount(sendAmount)
              .multiplyByDecimals(selectedSendAsset.decimals)
              .toHex()
    })

    resetSendFields()
  }, [
    selectedSendAsset,
    selectedAccount,
    selectedNetwork,
    sendAmount,
    toAddressOrUrl,
    showResolvedDomain,
    resolvedDomainAddress,
    resetSendFields
  ])

  const setSelectedAccountAndNetwork = React.useCallback(async () => {
    if (!chainId || !selectedAssetFromParams || !accountFromParams) {
      return
    }

    try {
      await setSelectedAccount(accountFromParams.accountId)
      await setNetwork({
        chainId: chainId,
        coin: selectedAssetFromParams.coin
      })
    } catch (e) {
      console.error(e)
    }
  }, [accountFromParams, chainId, selectedAssetFromParams])

  const handleInputAmountChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSendAmount(event.target.value)
    },
    []
  )

  const handleInputAddressChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setToAddressOrUrl(event.target.value)
    },
    [setToAddressOrUrl, addressWidthRef]
  )

  const onSelectSendOption = React.useCallback(
    (option: SendPageTabHashes) => {
      selectSendAsset(undefined)
      history.push(`${WalletRoutes.SendPageStart}${option}`)
    },
    [selectSendAsset]
  )

  const onENSConsent = React.useCallback(() => {
    enableEnsOffchainLookup()
    dismissOffchainEnsWarning(true)
  }, [enableEnsOffchainLookup])

  const setPresetAmountValue = React.useCallback(
    (percent: number) => {
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
    },
    [selectedSendAsset, selectedAccount, tokenBalancesRegistry]
  )

  // Modals
  const {
    closeModal: closeChecksumModal,
    openModal: openChecksumModal,
    ref: checksumInfoModalRef,
    isModalShown: showChecksumInfoModal
  } = useModal()
  const {
    closeModal: closeSelectTokenModal,
    openModal: openSelectTokenModal,
    ref: selectTokenModalRef,
    isModalShown: showSelectTokenModal
  } = useModal()

  // Effects
  React.useLayoutEffect(() => {
    // Update loading icon position when to-address changes.
    // Using an effect instead of within an on-change
    // because we want to have the latest text width from the dom
    const position = addressWidthRef.current?.clientWidth
    setDomainPosition(position ? position + 28 : 0)
  }, [toAddressOrUrl])

  React.useEffect(() => {
    // check if the user has selected an asset
    if (!selectedAssetFromParams || selectedSendAsset) {
      return
    }
    setSelectedAccountAndNetwork()
    selectSendAsset(selectedAssetFromParams)
  }, [
    selectSendAsset,
    selectedSendAsset,
    selectedAssetFromParams,
    setSelectedAccountAndNetwork
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
        cardHeader={<SendPageHeader />}
      >
        <SendContainer>
          <Row rowWidth='full' marginBottom={16}>
            <SelectSendOptionButton
              selectedSendOption={selectedSendOption}
              onClick={onSelectSendOption}
            />
          </Row>
          <SectionBox minHeight={150} hasError={insufficientFundsError}>
            {selectedSendOption === SendPageTabHashes.token && (
              <Column
                columnHeight='full'
                columnWidth='full'
                verticalAlign='space-between'
                horizontalAlign='space-between'
              >
                <Row rowWidth='full' horizontalAlign='flex-end'>
                  {isLoadingSelectedAccount || isLoadingBalances ? (
                    <SmallLoadingRing />
                  ) : (
                    <Text
                      textSize='14px'
                      textColor='text03'
                      maintainHeight={true}
                      isBold={true}
                    >
                      {accountNameAndBalance}
                    </Text>
                  )}
                </Row>
                <Row rowWidth='full'>
                  <Row>
                    <SelectTokenButton
                      onClick={openSelectTokenModal}
                      token={selectedSendAsset}
                      selectedSendOption={selectedSendOption}
                    />
                    {selectedSendOption === SendPageTabHashes.token &&
                      selectedSendAsset && (
                        <>
                          <HorizontalDivider
                            height={28}
                            marginLeft={8}
                            marginRight={8}
                            dividerTheme='lighter'
                          />
                          <PresetButton
                            buttonText={getLocale('braveWalletSendHalf')}
                            onClick={() => setPresetAmountValue(0.5)}
                          />
                          <PresetButton
                            buttonText={getLocale('braveWalletSendMax')}
                            onClick={() => setPresetAmountValue(1)}
                          />
                        </>
                      )}
                  </Row>
                  {selectedSendOption === SendPageTabHashes.token && (
                    <AmountInput
                      placeholder='0.0'
                      hasError={insufficientFundsError}
                      value={sendAmount}
                      onChange={handleInputAmountChange}
                    />
                  )}
                </Row>
                <Row rowWidth='full' horizontalAlign='flex-end'>
                  {isLoadingSpotPrices || isLoadingBalances ? (
                    <SmallLoadingRing />
                  ) : (
                    <Text
                      textSize='14px'
                      textColor='text03'
                      maintainHeight={true}
                      isBold={false}
                    >
                      {sendAmountFiatValue}
                    </Text>
                  )}
                </Row>
              </Column>
            )}
            {selectedSendOption === SendPageTabHashes.nft && (
              <Column
                columnWidth='full'
                columnHeight='full'
              >
                {accountNameAndBalance &&
                  <Row
                    horizontalAlign='flex-end'
                    rowWidth='full'
                    marginBottom={12}
                  >
                    <Text
                      textSize='14px'
                      textColor='text03'
                      maintainHeight={true}
                      isBold={true}
                      textAlign='right'
                    >
                      {accountNameAndBalance}
                    </Text>
                  </Row>
                }
                <Row
                  rowHeight='full'
                  rowWidth='full'
                  horizontalAlign='flex-start'
                  verticalAlign='center'
                  paddingLeft={8}
                >
                  <SelectTokenButton
                    onClick={openSelectTokenModal}
                    token={selectedSendAsset}
                    selectedSendOption={selectedSendOption} />
                </Row>
              </Column>
            )}
          </SectionBox>
          <SectionBox
            hasError={hasAddressError}
            hasWarning={hasAddressWarning}
            noPadding={true}
          >
            <InputRow
              rowWidth='full'
              verticalAlign='center'
              paddingTop={16}
              paddingBottom={showResolvedDomain ? 4 : 16}
              horizontalPadding={16}
            >
              {isSearchingForDomain && (
                <DomainLoadIcon position={domainPosition} />
              )}
              <DIVForWidth ref={addressWidthRef}>{toAddressOrUrl}</DIVForWidth>
              <AddressInput
                placeholder={getLocale('braveWalletEnterRecipientAddress')}
                hasError={hasAddressError}
                value={toAddressOrUrl}
                onChange={handleInputAddressChange}
                spellCheck={false}
                disabled={!selectedSendAsset}
              />
              <AccountSelector
                asset={selectedSendAsset}
                disabled={!selectedSendAsset}
                onSelectAddress={setToAddressOrUrl}
              />
            </InputRow>
            {showResolvedDomain && (
              <CopyAddress address={resolvedDomainAddress} />
            )}
            {addressMessageInformation && (
              <AddressMessage
                addressMessageInfo={addressMessageInformation}
                onClickHowToSolve={
                  addressErrorLocaleKey ===
                    braveWalletNotValidChecksumAddressError ||
                  addressWarningLocaleKey ===
                    braveWalletAddressMissingChecksumInfoWarning
                    ? openChecksumModal
                    : undefined
                }
              />
            )}
          </SectionBox>
          {showEnsOffchainWarning && !isOffChainEnsWarningDismissed ? (
            <StandardButton
              // This is always enabled to allow off-chain ENS lookups
              buttonText={getLocale('braveWalletEnsOffChainButton')}
              onClick={onENSConsent}
              buttonType='primary'
              buttonWidth='full'
              isLoading={isSearchingForDomain}
              hasError={reviewButtonHasError}
            />
          ) : (
            <StandardButton
              buttonText={getLocale(
                getReviewButtonText(
                  isSearchingForDomain,
                  sendAmountValidationError,
                  insufficientFundsError,
                  addressErrorLocaleKey,
                  addressWarningLocaleKey
                )
              ).replace('$1', CoinTypesMap[selectedNetwork?.coin ?? 0])}
              onClick={submitSend}
              buttonType='primary'
              buttonWidth='full'
              isLoading={isSearchingForDomain}
              disabled={
                isSearchingForDomain ||
                !toAddressOrUrl ||
                insufficientFundsError ||
                Boolean(addressError) ||
                sendAmount === '' ||
                parseFloat(sendAmount) === 0 ||
                Boolean(sendAmountValidationError)
              }
              hasError={reviewButtonHasError}
            />
          )}
        </SendContainer>
      </WalletPageWrapper>
      {showSelectTokenModal ? (
        <SelectTokenModal
          onClose={closeSelectTokenModal}
          selectedSendOption={selectedSendOption}
          ref={selectTokenModalRef}
          selectSendAsset={selectSendAsset}
        />
      ) : null}
      {showChecksumInfoModal ? (
        <ChecksumInfoModal
          onClose={closeChecksumModal}
          ref={checksumInfoModalRef}
        />
      ) : null}
    </>
  )
})

export default SendScreen

const SendPageHeader = React.memo(() => {
  return <PageTitleHeader title={getLocale('braveWalletSend')} />
})

/**
 * ETH → Wei conversion
 */
function ethToWeiAmount(
  sendAmount: string,
  selectedSendAsset: BraveWallet.BlockchainToken
): Amount {
  return new Amount(sendAmount).multiplyByDecimals(selectedSendAsset.decimals)
}

function getAddressMessageInfo({
  addressErrorKey,
  addressWarningKey,
  fevmTranslatedAddresses,
  showEnsOffchainWarning,
  showFilecoinFEVMWarning,
  toAddressOrUrl
}: {
  showFilecoinFEVMWarning: boolean
  fevmTranslatedAddresses:
    | Map<string, { address: string; fvmAddress: string }>
    | undefined
  toAddressOrUrl: string
  showEnsOffchainWarning: boolean
  addressErrorKey: string | undefined
  addressWarningKey: string | undefined
}): () =>
  | AddressMessageInfo
  | {
      placeholder: any
      title: string
      description?: string | undefined
      url?: string | undefined
      type?: 'error' | 'warning' | undefined
    }
  | undefined {
  return () => {
    if (showFilecoinFEVMWarning) {
      return {
        ...FEVMAddressConvertionMessage,
        placeholder: fevmTranslatedAddresses?.[toAddressOrUrl]
      }
    }
    if (showEnsOffchainWarning) {
      return ENSOffchainLookupMessage
    }
    if (addressErrorKey === 'braveWalletNotValidChecksumAddressError') {
      return ErrorFailedChecksumMessage
    }

    if (addressWarningKey === 'braveWalletAddressMissingChecksumInfoWarning') {
      return WarningFailedChecksumMessage
    }
    return undefined
  }
}

function getReviewButtonText(
  searchingForDomain: boolean,
  sendAmountValidationError: string | undefined,
  insufficientFundsError: boolean,
  addressError: string | undefined,
  addressWarningKey: string | undefined
) {
  if (searchingForDomain) {
    return 'braveWalletSearchingForDomain'
  }
  if (sendAmountValidationError) {
    return 'braveWalletDecimalPlacesError'
  }
  if (insufficientFundsError) {
    return 'braveWalletNotEnoughFunds'
  }
  if (
    addressError &&
    addressError !== 'braveWalletNotValidChecksumAddressError'
  ) {
    return addressError
  }

  if (
    addressWarningKey &&
    addressWarningKey !== 'braveWalletAddressMissingChecksumInfoWarning'
  ) {
    return addressWarningKey
  }

  return 'braveWalletReviewSend'
}

const processDomainLookupResponseWarning = (
  urlHasValidExtension: boolean,
  resolvedAddress: string | undefined,
  hasDomainLookupError: boolean,
  requireOffchainConsent: boolean,
  selectedAccountAddress?: string,
) => {
  if (requireOffchainConsent) {
    // handled separately
    return undefined
  }

  if (!urlHasValidExtension) {
    return 'braveWalletInvalidRecipientAddress'
  }

  if (
    hasDomainLookupError ||
    !resolvedAddress
  ) {
    return 'braveWalletNotDomain'
  }

  // If found address is the same as the selectedAccounts Wallet Address
  if (
    selectedAccountAddress &&
    resolvedAddress.toLowerCase() === selectedAccountAddress.toLowerCase()
  ) {
    return 'braveWalletSameAddressError'
  }

  return undefined
}

const validateETHAddress = (address: string, checksumAddress: string) => {
  if (!isValidEVMAddress(address)) {
    return 'braveWalletInvalidRecipientAddress'
  }

  return checksumAddress &&
    checksumAddress !== address &&
    [address.toLowerCase(), address.toUpperCase()].includes(address)
    ? 'braveWalletNotValidChecksumAddressError'
    : undefined
}

const processEthereumAddress = (
  addressOrUrl: string,
  selectedSendAsset: BraveWallet.BlockchainToken | undefined,
  checksumAddress: string
) => {
  const valueToLowerCase = addressOrUrl.toLowerCase()

  if (
    selectedSendAsset &&
    (selectedSendAsset.chainId ===
      BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID ||
      selectedSendAsset.chainId ===
        BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID) &&
    isValidFilAddress(addressOrUrl)
  ) {
    return undefined
  }

  // If value starts with 0x, will check if it's a valid address
  if (valueToLowerCase.startsWith('0x')) {
    return validateETHAddress(addressOrUrl, checksumAddress)
  }

  // Fallback error state
  return valueToLowerCase === ''
    ? undefined
    : 'braveWalletInvalidRecipientAddress'
}

const processFilecoinAddress = (
  addressOrUrl: string,
  checksum: string
) => {
  const valueToLowerCase = addressOrUrl.toLowerCase()

  // If value starts with 0x, will check if it's a valid address
  if (valueToLowerCase.startsWith('0x')) {
    return validateETHAddress(addressOrUrl, checksum)
  }

  if (!isValidFilAddress(valueToLowerCase)) {
    return 'braveWalletInvalidRecipientAddress'
  }

  // Default
  return undefined
}

const processSolanaAddress = (
  addressOrUrl: string,
  isBase58Encoded: boolean | undefined
) => {
  // Check if value is a Base58 Encoded Solana Pubkey
  if (!isBase58Encoded) {
    return 'braveWalletInvalidRecipientAddress'
  }

  return undefined
}

const processBitcoinAddress = (addressOrUrl: string) => {
  // Check if value is the same as the sending address
  // TODO(apaymyshev): should prohibit self transfers?

  // TODO(apaymyshev): validate address format.
  return undefined
}

function processAddressOrUrl({
  addressOrUrl,
  ethAddressChecksum,
  isBase58,
  coinType,
  selectedSendAsset,
}: {
  addressOrUrl: string
  coinType: BraveWallet.CoinType | undefined
  selectedSendAsset: BraveWallet.BlockchainToken | undefined
  ethAddressChecksum: string
  isBase58: boolean
}) {
  // Do nothing if value is an empty string
  if (addressOrUrl === '') {
    return undefined
  }

  switch (coinType) {
    case undefined: return undefined
    case BraveWallet.CoinType.ETH: {
      return processEthereumAddress(
        addressOrUrl,
        selectedSendAsset,
        ethAddressChecksum
      )
    }
    case BraveWallet.CoinType.FIL: {
      return processFilecoinAddress(addressOrUrl, ethAddressChecksum)
    }
    case BraveWallet.CoinType.SOL: {
      return processSolanaAddress(addressOrUrl, isBase58)
    }
    case BraveWallet.CoinType.BTC: {
      return processBitcoinAddress(addressOrUrl)
    }
    default: {
      console.log(`Unknown coin ${coinType}`)
      return undefined
    }
  }
}
