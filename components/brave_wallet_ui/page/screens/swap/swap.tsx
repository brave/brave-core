// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

// State
import { useGetSelectedChainQuery } from '../../../common/slices/api.slice'

// Types
import { BraveWallet } from '../../../constants/types'

// Constants
import { BRAVE_SWAP_DATA_THEME_KEY } from './constants/magics'

// Hooks
import { useSwap } from './hooks/useSwap'
import { useOnClickOutside } from '../../../common/hooks/useOnClickOutside'

// Utils
import { getLocale } from '$web-common/locale'

// Components
// FIXME(douglashdaniel): fix imports
import { StandardButton } from './components/buttons/standard-button/standard-button'
import { FlipTokensButton } from './components/buttons/flip-tokens-button/flip-tokens-button'
import { SwapContainer } from './components/swap/swap-container/swap-container'
import { FromSection } from './components/swap/from-section/from-section'
import { ToSection } from './components/swap/to-section/to-section'
import { SelectTokenModal } from './components/swap/select-token-modal/select-token-modal'
import { QuoteInfo } from './components/swap/quote-info/quote-info'
import { QuoteOptions } from './components/swap/quote-options/quote-options'
import { SwapSettingsModal } from './components/swap/settings/swap-settings-modal'
// import { SwapSkeleton } from './components/swap/swap-skeleton/swap-skeleton'
import { PrivacyModal } from './components/swap/privacy-modal/privacy-modal'

import { SwapSectionBox } from './components/boxes/swap-section-box'

// Assets
// FIXME(douglashdaniel): This is not the correct icon
import AdvancedIcon from './assets/lp-icons/0x.svg'

// Styled Components
import { Row, Text, IconButton, StyledDiv } from './components/shared-swap.styles'

export const Swap = () => {
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // Hooks
  const swap = useSwap()
  const {
    fromAmount,
    toAmount,
    fromToken,
    toToken,
    isFetchingQuote,
    quoteOptions,
    selectedQuoteOptionIndex,
    selectingFromOrTo,
    fromAssetBalance,
    fiatValue,
    selectedGasFeeOption,
    slippageTolerance,
    useDirectRoute,
    gasEstimates,
    getCachedAssetBalance,
    onSelectFromToken,
    onSelectToToken,
    onSelectQuoteOption,
    onClickFlipSwapTokens,
    setSelectingFromOrTo,
    handleOnSetFromAmount,
    handleOnSetToAmount,
    setSelectedGasFeeOption,
    setSlippageTolerance,
    setUseDirectRoute,
    onSubmit,
    submitButtonText,
    isSubmitButtonDisabled,
    swapValidationError,
    refreshBlockchainState,
    getNetworkAssetsList
  } = swap

  // State
  const [showSwapSettings, setShowSwapSettings] = React.useState<boolean>(false)
  const [showPrivacyModal, setShowPrivacyModal] = React.useState<boolean>(false)

  // Refs
  const selectTokenModalRef = React.useRef<HTMLDivElement>(null)
  const swapSettingsModalRef = React.useRef<HTMLDivElement>(null)
  const privacyModalRef = React.useRef<HTMLDivElement>(null)

  const onToggleShowSwapSettings = React.useCallback(() => {
    setShowSwapSettings((prev) => !prev)
    if (slippageTolerance === '') {
      setSlippageTolerance('0.5')
    }
  }, [slippageTolerance, setSlippageTolerance])

  // Hooks
  useOnClickOutside(
    selectTokenModalRef,
    () => setSelectingFromOrTo(undefined),
    selectingFromOrTo !== undefined
  )
  useOnClickOutside(
    swapSettingsModalRef,
    onToggleShowSwapSettings,
    showSwapSettings
  )
  useOnClickOutside(
    privacyModalRef,
    () => setShowPrivacyModal(false),
    showPrivacyModal
  )

  React.useEffect(() => {
    const userTheme = window.localStorage.getItem(BRAVE_SWAP_DATA_THEME_KEY)
    if (userTheme === null) {
      return
    }
    document.documentElement.setAttribute('data-theme', userTheme)
  }, [])

  // FIXME(douglashdaniel): this should be computed
  // if (!isReady) {
  //   return <SwapSkeleton />
  // }

  // render
  return (
    <>
      <SwapContainer
        showPrivacyModal={() => setShowPrivacyModal(true)}
        refreshBlockchainState={refreshBlockchainState}
      >
        <Row
          rowWidth='full'
          horizontalPadding={16}
          verticalPadding={6}
          marginBottom={18}
        >
          <Text isBold={true}>{getLocale('braveSwap')}</Text>
          <SettingsWrapper ref={swapSettingsModalRef}>
            <IconButton
              icon={AdvancedIcon}
              onClick={onToggleShowSwapSettings}
            />
            {showSwapSettings && (
              <SwapSettingsModal
                selectedGasFeeOption={selectedGasFeeOption}
                slippageTolerance={slippageTolerance}
                useDirectRoute={useDirectRoute}
                setSelectedGasFeeOption={setSelectedGasFeeOption}
                setSlippageTolerance={setSlippageTolerance}
                setUseDirectRoute={setUseDirectRoute}
                gasEstimates={gasEstimates}
                onClose={() => setShowSwapSettings(false)}
              />
            )}
          </SettingsWrapper>
        </Row>
        <FromSection
          onInputChange={handleOnSetFromAmount}
          inputValue={fromAmount}
          onClickSelectToken={() => setSelectingFromOrTo('from')}
          token={fromToken}
          tokenBalance={fromAssetBalance}
          hasInputError={
            swapValidationError === 'insufficientBalance' ||
            swapValidationError === 'fromAmountDecimalsOverflow'
          }
          fiatValue={fiatValue}
        />
        <FlipTokensButton onClick={onClickFlipSwapTokens} />
        <SwapSectionBox boxType='secondary'>
          <ToSection
            onClickSelectToken={() => setSelectingFromOrTo('to')}
            token={toToken}
            inputValue={toAmount}
            onInputChange={handleOnSetToAmount}
            hasInputError={swapValidationError === 'toAmountDecimalsOverflow'}
            isLoading={isFetchingQuote}
            disabled={selectedNetwork?.coin === BraveWallet.CoinType.SOL}
          />
          {selectedNetwork?.coin === BraveWallet.CoinType.SOL && quoteOptions.length > 0 && (
            <QuoteOptions
              options={quoteOptions}
              selectedQuoteOptionIndex={selectedQuoteOptionIndex}
              onSelectQuoteOption={onSelectQuoteOption}
            />
          )}
        </SwapSectionBox>
        {quoteOptions.length > 0 && (
          <>
            <QuoteInfo
              selectedQuoteOption={quoteOptions[selectedQuoteOptionIndex]}
              fromToken={fromToken}
              toToken={toToken}
              toAmount={toAmount}
            />

            {/* TODO: Swap and Send  is currently unavailable
              <SwapAndSend
                onChangeSwapAndSendSelected={setSwapAndSendSelected}
                handleOnSetToAnotherAddress={handleOnSetToAnotherAddress}
                onCheckUserConfirmedAddress={onCheckUserConfirmedAddress}
                onSelectSwapAndSendOption={onSetSelectedSwapAndSendOption}
                onSelectSwapSendAccount={setSelectedSwapSendAccount}
                swapAndSendSelected={swapAndSendSelected}
                selectedSwapAndSendOption={selectedSwapAndSendOption}
                selectedSwapSendAccount={selectedSwapSendAccount}
                toAnotherAddress={toAnotherAddress}
                userConfirmedAddress={userConfirmedAddress}
            */}
          </>
        )}
        <StandardButton
          onClick={onSubmit}
          buttonText={submitButtonText}
          buttonType='primary'
          buttonWidth='full'
          verticalMargin={16}
          disabled={isSubmitButtonDisabled}
        />
      </SwapContainer>
      {selectingFromOrTo && (
        <SelectTokenModal
          ref={selectTokenModalRef}
          onClose={() => setSelectingFromOrTo(undefined)}
          onSelectToken={
            selectingFromOrTo === 'from' ? onSelectFromToken : onSelectToToken
          }
          disabledToken={selectingFromOrTo === 'from' ? toToken : fromToken}
          getCachedAssetBalance={getCachedAssetBalance}
          selectingFromOrTo={selectingFromOrTo}
          refreshBlockchainState={refreshBlockchainState}
          getNetworkAssetsList={getNetworkAssetsList}
        />
      )}
      {showPrivacyModal && (
        <PrivacyModal
          ref={privacyModalRef}
          onClose={() => setShowPrivacyModal(false)}
        />
      )}
    </>
  )
}

const SettingsWrapper = styled(StyledDiv)`
  display: flex;
  position: relative;
`
