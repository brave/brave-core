// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// Types
import { BraveWallet, WalletRoutes } from '../../../constants/types'

// Hooks
import { useSwap } from './hooks/useSwap'
import { useOnClickOutside } from '../../../common/hooks/useOnClickOutside'

// Utils
import { getLocale } from '$web-common/locale'

// Components
import {
  StandardButton //
} from './components/buttons/standard-button/standard-button'
import { FromAsset } from '../composer_ui/from_asset/from_asset'
import { ToAsset } from '../composer_ui/to_asset/to_asset'
import { SelectTokenModal } from '../composer_ui/select_token_modal/select_token_modal'
import { QuoteInfo } from './components/swap/quote-info/quote-info'
import {
  AdvancedSettingsModal //
} from '../composer_ui/advanced_settings_modal.style.ts/advanced_settings_modal'
import { PrivacyModal } from './components/swap/privacy-modal/privacy-modal'
import { ComposerControls } from '../composer_ui/composer_controls/composer_controls'
import WalletPageWrapper from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import { DefaultPanelHeader } from '../../../components/desktop/card-headers/default-panel-header'

// Styled Components
import { Row } from '../../../components/shared/style'

export const Swap = () => {
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
    selectedGasFeeOption,
    slippageTolerance,
    gasEstimates,
    onSelectFromToken,
    onSelectToToken,
    onClickFlipSwapTokens,
    setSelectingFromOrTo,
    handleOnSetFromAmount,
    handleOnSetToAmount,
    setSelectedGasFeeOption,
    setSlippageTolerance,
    onSubmit,
    submitButtonText,
    isSubmitButtonDisabled,
    swapValidationError,
    spotPrices,
    selectedNetwork,
    setSelectedNetwork,
    selectedAccount,
    tokenBalancesRegistry,
    isLoadingBalances
  } = swap

  // State
  const [showSwapSettings, setShowSwapSettings] = React.useState<boolean>(false)
  const [showPrivacyModal, setShowPrivacyModal] = React.useState<boolean>(false)

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)

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

  // render
  return (
    <>
      <WalletPageWrapper
        wrapContentInBox={true}
        noCardPadding={true}
        noMinCardHeight={true}
        hideDivider={true}
        cardHeader={
          isPanel ? (
            <DefaultPanelHeader
              title={getLocale('braveWalletSwap')}
              expandRoute={WalletRoutes.Swap}
            />
          ) : undefined
        }
      >
        <FromAsset
          onInputChange={handleOnSetFromAmount}
          inputValue={fromAmount}
          onClickSelectToken={() => setSelectingFromOrTo('from')}
          token={fromToken}
          tokenBalancesRegistry={tokenBalancesRegistry}
          isLoadingBalances={isLoadingBalances}
          hasInputError={
            swapValidationError === 'insufficientBalance' ||
            swapValidationError === 'fromAmountDecimalsOverflow'
          }
          network={selectedNetwork}
          account={selectedAccount}
        />
        <ComposerControls
          onFlipAssets={onClickFlipSwapTokens}
          onOpenSettings={onToggleShowSwapSettings}
        />
        <ToAsset
          onInputChange={handleOnSetToAmount}
          inputValue={toAmount}
          onClickSelectToken={() => setSelectingFromOrTo('to')}
          token={toToken}
          inputDisabled={selectedNetwork?.coin === BraveWallet.CoinType.SOL}
          hasInputError={swapValidationError === 'toAmountDecimalsOverflow'}
          network={selectedNetwork}
          selectedSendOption='#token'
          isFetchingQuote={isFetchingQuote}
        >
          {/* TODO: QuoteOptions is currently unused
          selectedNetwork?.coin === BraveWallet.CoinType.SOL &&
            quoteOptions.length > 0 && (
              <QuoteOptions
                options={quoteOptions}
                selectedQuoteOptionIndex={selectedQuoteOptionIndex}
                onSelectQuoteOption={onSelectQuoteOption}
                spotPrices={spotPrices}
              />
          ) */}
          {quoteOptions.length > 0 && (
            <>
              <QuoteInfo
                selectedQuoteOption={quoteOptions[selectedQuoteOptionIndex]}
                fromToken={fromToken}
                toToken={toToken}
                toAmount={toAmount}
                spotPrices={spotPrices}
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
          <Row
            width='100%'
            padding='16px 16px 0px 16px'
          >
            <StandardButton
              onClick={onSubmit}
              buttonType='primary'
              buttonWidth='full'
              disabled={isSubmitButtonDisabled}
            >
              {submitButtonText}
            </StandardButton>
          </Row>
        </ToAsset>
        {showSwapSettings && (
          <AdvancedSettingsModal
            selectedGasFeeOption={selectedGasFeeOption}
            slippageTolerance={slippageTolerance}
            setSelectedGasFeeOption={setSelectedGasFeeOption}
            setSlippageTolerance={setSlippageTolerance}
            gasEstimates={gasEstimates}
            onClose={() => setShowSwapSettings(false)}
            selectedNetwork={selectedNetwork}
            ref={swapSettingsModalRef}
          />
        )}
      </WalletPageWrapper>
      {selectingFromOrTo && (
        <SelectTokenModal
          ref={selectTokenModalRef}
          onClose={() => setSelectingFromOrTo(undefined)}
          onSelectAsset={
            selectingFromOrTo === 'from' ? onSelectFromToken : onSelectToToken
          }
          selectedToken={selectingFromOrTo === 'from' ? toToken : fromToken}
          selectedSendOption='#token'
          selectedNetwork={selectedNetwork}
          setSelectedNetwork={setSelectedNetwork}
          showFullFlatTokenList={selectingFromOrTo === 'to'}
          modalType='swap'
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
