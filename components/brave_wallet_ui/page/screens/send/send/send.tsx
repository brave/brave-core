// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet, SendOptionTypes } from '../../../../constants/types'

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
import { getTokensNetwork } from '../../../../utils/network-utils'
import { reduceAddress } from '../../../../utils/reduce-address'
import { endsWithAny } from '../../../../utils/string-utils'

// Hooks
import useSend from '../../../../common/hooks/send'
import { usePreset } from '../../../../common/hooks'

// Styled Components
import {
  SendContainer,
  SectionBox,
  AddressInput,
  AmountInput,
  Background,
  FoundAddress,
  DIVForWidth
} from './send.style'
import { Column, Text, Row, HorizontalDivider } from '../shared.styles'

// Component
import { SelectSendOptionButton } from '../components/select-send-option-button/select-send-option-button'
import { StandardButton } from '../components/standard-button/standard-button'
import { SelectTokenButton } from '../components/select-token-button/select-token-button'
import { PresetButton } from '../components/preset-button/preset-button'
import { AccountSelector } from '../components/account-selector/account-selector'

interface Props {
  onShowSelectTokenModal: () => void
  selectedSendOption: SendOptionTypes
  setSelectedSendOption: (sendOption: SendOptionTypes) => void
}

const INPUT_WIDTH_ID = 'input-width'

export const Send = (props: Props) => {
  const { onShowSelectTokenModal, setSelectedSendOption, selectedSendOption } = props

  // Wallet Selectors
  const selectedAccount = useUnsafeWalletSelector(WalletSelectors.selectedAccount)
  const spotPrices = useUnsafeWalletSelector(WalletSelectors.transactionSpotPrices)
  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)
  const networks = useUnsafeWalletSelector(WalletSelectors.networkList)

  const {
    toAddressOrUrl,
    toAddress,
    // ToDo: We need mockup designs for offchain lookup.
    // showEnsOffchainLookupOptions,
    // ensOffchainLookupOptions,
    // setEnsOffchainLookupOptions,
    addressError,
    addressWarning,
    sendAmount,
    selectedSendAsset,
    sendAmountValidationError,
    setSendAmount,
    setToAddressOrUrl,
    submitSend,
    selectSendAsset
  } = useSend(true)

  // Hooks
  const onSelectPresetAmount = usePreset(
    {
      onSetAmount: setSendAmount,
      asset: selectedSendAsset
    }
  )

  // Refs
  const ref = React.createRef<HTMLDivElement>()

  // State
  const [backgroundHeight, setBackgroundHeight] = React.useState<number>(0)
  const [backgroundOpacity, setBackgroundOpacity] = React.useState<number>(0.3)
  const [foundAddressPosition, setFoundAddressPosition] = React.useState<number>(0)

  // Methods
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
    []
  )

  const setPresetAmountValue = React.useCallback((percent: number) => {
    onSelectPresetAmount(percent)
  }, [onSelectPresetAmount])

  const onSelectSendOption = React.useCallback((option: SendOptionTypes) => {
    selectSendAsset(undefined)
    setSelectedSendOption(option)
  }, [selectedSendAsset])

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
        .toHex()
    }).formatAsFiat(defaultCurrencies.fiat)
  }, [spotPrices, selectedSendAsset, sendAmount, defaultCurrencies.fiat, sendAssetBalance, selectedSendOption])

  const reviewButtonText = React.useMemo(() => {
    return sendAmountValidationError
      ? getLocale('braveWalletDecimalPlacesError')
      : insufficientFundsError
        ? getLocale('braveWalletNotEnoughFunds')
        : (addressError !== undefined && addressError !== '')
          ? addressError
          : (addressWarning !== undefined && addressWarning !== '')
            ? addressWarning
            : getLocale('braveWalletReviewOrder')
  }, [insufficientFundsError, addressError, addressWarning, sendAmountValidationError])

  const isReviewButtonDisabled = React.useMemo(() => {
    return toAddressOrUrl === '' ||
      parseFloat(sendAmount) === 0 ||
      sendAmount === '' ||
      insufficientFundsError ||
      (addressError !== undefined && addressError !== '') ||
      sendAmountValidationError !== undefined
  }, [toAddressOrUrl, sendAmount, insufficientFundsError, addressError, sendAmountValidationError])

  const selectedTokensNetwork = React.useMemo(() => {
    if (selectedSendAsset) {
      return getTokensNetwork(networks, selectedSendAsset)
    }
    return undefined
  }, [selectedSendAsset, networks])

  const showResolvedDomainAddress = React.useMemo(() => {
    if (
      (addressError === undefined || addressError === '' || addressError === getLocale('braveWalletSameAddressError')) &&
      toAddress &&
      endsWithAny(allSupportedExtensions, toAddressOrUrl.toLowerCase())
    ) {
      setFoundAddressPosition(document.getElementById(INPUT_WIDTH_ID)?.clientWidth ?? 0)
      return true
    }
    return false
  }, [addressError, toAddress, toAddressOrUrl])

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
          lessLeftPadding={true}
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
                <Text textSize='14px' textColor='text03' maintainHeight={true} isBold={true}>
                  {accountNameAndBalance}
                </Text>
              </Column>
            </Row>
          }
        </SectionBox>
        <SectionBox hasError={false} boxDirection='row'>
          {showResolvedDomainAddress &&
            <FoundAddress
              textSize='16px'
              textColor='text03'
              isBold={false}
              position={foundAddressPosition + 22}
            >
              {reduceAddress(toAddress)}
            </FoundAddress>
          }
          <DIVForWidth id={INPUT_WIDTH_ID}>{toAddressOrUrl}</DIVForWidth>
          <AddressInput
            placeholder={getLocale('braveWalletEnterRecipientAddress')}
            hasError={
              (addressError !== undefined && addressError !== '') ||
              (addressWarning !== undefined && addressWarning !== '')
            }
            value={toAddressOrUrl}
            onChange={handleInputAddressChange}
            spellCheck={false}
          />
          <AccountSelector onSelectAddress={setToAddressOrUrl} />
        </SectionBox>
        <StandardButton
          buttonText={reviewButtonText}
          onClick={submitSend}
          buttonType='primary'
          buttonWidth='full'
          disabled={isReviewButtonDisabled}
          hasError={
            insufficientFundsError ||
            (addressError !== undefined && addressError !== '')
          }
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
    </>
  )
}

export default Send
