// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Messages
import { ENSOffchainLookupMessage, FailedChecksumMessage } from '../send-ui-messages'

// Types
import { BraveWallet, SendOptionTypes, AddressMessageInfo } from '../../../../constants/types'

// Selectors
import { WalletSelectors } from '../../../../common/selectors'
import { useUnsafeWalletSelector } from '../../../../common/hooks/use-safe-selector'

// Constants
import { allSupportedExtensions } from '../../../../common/constants/domain-extensions'

// Utils
import { getLocale } from '../../../../../common/locale'
import Amount from '../../../../utils/amount'
import { getBalance, formatTokenBalanceWithSymbol } from '../../../../utils/balance-utils'
import { computeFiatAmount } from '../../../../utils/pricing-utils'
import { endsWithAny } from '../../../../utils/string-utils'

// Hooks
import { usePreset, useBalanceUpdater, useSend } from '../../../../common/hooks'
import { useOnClickOutside } from '../../../../common/hooks/useOnClickOutside'
import { useGetNetworkQuery } from '../../../../common/slices/api.slice'

// Styled Components
import {
  SendContainer,
  SectionBox,
  AddressInput,
  AmountInput,
  Background,
  DIVForWidth,
  InputRow,
  DomainLoadIcon
} from './send.style'
import { Column, Text, Row, HorizontalDivider } from '../shared.styles'

// Component
import { SelectSendOptionButton } from '../components/select-send-option-button/select-send-option-button'
import { StandardButton } from '../components/standard-button/standard-button'
import { SelectTokenButton } from '../components/select-token-button/select-token-button'
import { PresetButton } from '../components/preset-button/preset-button'
import { AccountSelector } from '../components/account-selector/account-selector'
import { AddressMessage } from '../components/address-message/address-message'
import { SelectTokenModal } from '../components/select-token-modal/select-token-modal'
import { CopyAddress } from '../components/copy-address/copy-address'
import { ChecksumInfoModal } from '../components/checksum-info-modal/checksum-info-modal'

interface Props {
  onShowSelectTokenModal: () => void
  onHideSelectTokenModal: () => void
  selectedSendOption: SendOptionTypes
  setSelectedSendOption: (sendOption: SendOptionTypes) => void
  selectTokenModalRef: React.RefObject<HTMLDivElement>
  showSelectTokenModal: boolean
}

export const Send = (props: Props) => {
  const {
    onShowSelectTokenModal,
    setSelectedSendOption,
    selectedSendOption,
    onHideSelectTokenModal,
    selectTokenModalRef,
    showSelectTokenModal
  } = props

  // Wallet Selectors
  const selectedAccount = useUnsafeWalletSelector(WalletSelectors.selectedAccount)
  const spotPrices = useUnsafeWalletSelector(WalletSelectors.transactionSpotPrices)
  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)

  // Hooks
  useBalanceUpdater()

  const {
    toAddressOrUrl,
    toAddress,
    enableEnsOffchainLookup,
    showEnsOffchainWarning,
    setShowEnsOffchainWarning,
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
  } = useSend(true)

  // Queries
  const { data: selectedTokensNetwork } = useGetNetworkQuery(
    selectedSendAsset,
    { skip: !selectedSendAsset }
  )

  // Refs
  const ref = React.createRef<HTMLDivElement>()
  const checksumInfoModalRef = React.useRef<HTMLDivElement>(null)

  // State
  const [backgroundHeight, setBackgroundHeight] = React.useState<number>(0)
  const [backgroundOpacity, setBackgroundOpacity] = React.useState<number>(0.3)
  const [domainPosition, setDomainPosition] = React.useState<number>(0)
  const [showChecksumInfoModal, setShowChecksumInfoModal] = React.useState<boolean>(false)


  const onSelectPresetAmount = usePreset(
    {
      onSetAmount: setSendAmount,
      asset: selectedSendAsset
    }
  )

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

  const setPresetAmountValue = React.useCallback((percent: number) => {
    onSelectPresetAmount(percent)
  }, [onSelectPresetAmount])

  const onSelectSendOption = React.useCallback((option: SendOptionTypes) => {
    selectSendAsset(undefined)
    setSelectedSendOption(option)
  }, [selectedSendAsset])

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

  // Memos
  const sendAssetBalance = React.useMemo(() => {
    return getBalance(selectedAccount, selectedSendAsset)
  }, [selectedAccount, selectedSendAsset])

  const accountNameAndBalance = React.useMemo(() => {
    if (!selectedSendAsset || sendAssetBalance === '') {
      return ''
    }
    if (selectedSendOption === 'nft') {
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

  const sendAmountFiatValue = React.useMemo(() => {
    if (!selectedSendAsset || sendAssetBalance === '' || selectedSendOption === 'nft') {
      return ''
    }
    return computeFiatAmount(spotPrices, {
      decimals: selectedSendAsset.decimals,
      symbol: selectedSendAsset.symbol,
      value: new Amount(sendAmount !== '' ? sendAmount : '0')
        .multiplyByDecimals(selectedSendAsset.decimals) // ETH → Wei conversion
        .toHex(),
      contractAddress: selectedSendAsset.contractAddress,
      chainId: selectedSendAsset.chainId
    }).formatAsFiat(defaultCurrencies.fiat)
  }, [spotPrices, selectedSendAsset, sendAmount, defaultCurrencies.fiat, sendAssetBalance, selectedSendOption])

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
  }, [showEnsOffchainWarning, addressError, addressWarning])

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

  // Effects
  React.useEffect(() => {
    // Keeps track of the Swap Containers Height to update
    // the network backgrounds height.
    setBackgroundHeight(ref?.current?.clientHeight ?? 0)
  }, [ref?.current?.clientHeight])

  React.useEffect(() => {
    let subscribed = true
    // Changes network background opacity to 0.6 after changing networks
    setBackgroundOpacity(0.6)
    // Changes network background opacity back to 0.3 after 1 second
    setTimeout(() => {
      if (subscribed) {
        setBackgroundOpacity(0.3)
      }
    }, 1000)

    // cleanup
    return () => {
      subscribed = false
    }
  }, [selectedTokensNetwork])

  // render
  return (
    <>
      <SendContainer ref={ref}>
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
          {selectedSendOption === 'token' &&
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
                    onClick={onShowSelectTokenModal}
                    token={selectedSendAsset}
                    selectedSendOption={selectedSendOption} />
                  {selectedSendOption === 'token' && selectedSendAsset &&
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
                {selectedSendOption === 'token' &&
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
          {selectedSendOption === 'nft' &&
            <Row
              rowWidth='full'
              rowHeight='full'
            >
              <Column
                columnHeight='full'
                verticalAlign='center'
              >
                <SelectTokenButton
                  onClick={onShowSelectTokenModal}
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
            <AccountSelector disabled={!selectedSendAsset} onSelectAddress={updateToAddressOrUrl} />
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
      <Background
        backgroundOpacity={backgroundOpacity}
        height={backgroundHeight}
        network={
          selectedTokensNetwork?.chainId === BraveWallet.LOCALHOST_CHAIN_ID
            ? `${selectedTokensNetwork?.chainId}${selectedTokensNetwork?.coin}`
            : selectedTokensNetwork?.chainId ?? ''
        }
      />
      {showSelectTokenModal &&
        <SelectTokenModal
          onClose={onHideSelectTokenModal}
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
