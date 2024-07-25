// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// Types
import {
  WalletRoutes,
  BraveWallet,
  SwapProviderNameMapping
} from '../../../constants/types'

// Hooks
import { useSwap } from './hooks/useSwap'
import { useOnClickOutside } from '../../../common/hooks/useOnClickOutside'

// Utils
import { getLocale } from '$web-common/locale'

// Components
import { FromAsset } from '../composer_ui/from_asset/from_asset'
import { ToAsset } from '../composer_ui/to_asset/to_asset'
import { SelectTokenModal } from '../composer_ui/select_token_modal/select_token_modal'
import { QuoteInfo } from './components/swap/quote-info/quote-info'
import { PrivacyModal } from './components/swap/privacy-modal/privacy-modal'
import { ComposerControls } from '../composer_ui/composer_controls/composer_controls'
import WalletPageWrapper from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import { PanelActionHeader } from '../../../components/desktop/card-headers/panel-action-header'
import { SwapProviders } from './components/swap/swap_providers/swap_providers'
import {
  BottomSheet //
} from '../../../components/shared/bottom_sheet/bottom_sheet'
import { PopupModal } from '../../../components/desktop/popup-modals/index'

// Styled Components
import {
  Column,
  LeoSquaredButton,
  VerticalSpace
} from '../../../components/shared/style'
import {
  ReviewButtonRow,
  AlertMessage,
  AlertMessageButton,
  AlertMessageWrapper
} from '../composer_ui/shared_composer.style'

export const Swap = () => {
  // Hooks
  const swap = useSwap()
  const {
    fromNetwork,
    fromToken,
    fromAccount,
    fromAmount,
    toNetwork,
    toToken,
    toAmount,
    isFetchingQuote,
    quoteOptions,
    selectedQuoteOptionId,
    selectingFromOrTo,
    slippageTolerance,
    onSelectFromToken,
    onSelectToToken,
    onClickFlipSwapTokens,
    setSelectingFromOrTo,
    handleOnSetFromAmount,
    handleOnSetToAmount,
    handleQuoteRefresh,
    onChangeSlippageTolerance,
    onSubmit,
    onChangeRecipient,
    onChangeSwapProvider,
    onSelectQuoteOption,
    submitButtonText,
    isSubmitButtonDisabled,
    swapValidationError,
    tokenBalancesRegistry,
    isLoadingBalances,
    swapFees,
    isBridge,
    toAccount,
    timeUntilNextQuote,
    selectedProvider,
    availableProvidersForSwap,
    isSubmittingSwap
  } = swap

  // State
  const [showSwapProviders, setShowSwapProviders] =
    React.useState<boolean>(false)
  const [showPrivacyModal, setShowPrivacyModal] = React.useState<boolean>(false)

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // Refs
  const selectTokenModalRef = React.useRef<HTMLDivElement>(null)
  const privacyModalRef = React.useRef<HTMLDivElement>(null)

  // Methods
  const handleOnChangeSwapProvider = React.useCallback(
    (provider: BraveWallet.SwapProvider) => {
      onChangeSwapProvider(provider)
      setShowSwapProviders(false)
    },
    [onChangeSwapProvider]
  )

  // Hooks
  useOnClickOutside(
    selectTokenModalRef,
    () => setSelectingFromOrTo(undefined),
    selectingFromOrTo !== undefined
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
        hideNav={isPanel}
        cardHeader={
          isPanel ? (
            <PanelActionHeader
              title={
                isBridge
                  ? getLocale('braveWalletBridge')
                  : getLocale('braveWalletSwap')
              }
              expandRoute={isBridge ? WalletRoutes.Bridge : WalletRoutes.Swap}
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
          network={fromNetwork}
          account={fromAccount}
        />
        <ComposerControls
          onFlipAssets={onClickFlipSwapTokens}
          onOpenProviders={() => setShowSwapProviders(true)}
          selectedProvider={selectedProvider}
          flipAssetsDisabled={!fromToken || !toToken}
        />
        <ToAsset
          onInputChange={handleOnSetToAmount}
          onRefreshQuote={handleQuoteRefresh}
          inputValue={toAmount}
          onClickSelectToken={() => setSelectingFromOrTo('to')}
          token={toToken}
          inputDisabled={false}
          hasInputError={swapValidationError === 'toAmountDecimalsOverflow'}
          network={toNetwork}
          selectedSendOption='#token'
          isFetchingQuote={isFetchingQuote}
          buttonDisabled={!fromToken}
          timeUntilNextQuote={timeUntilNextQuote}
        >
          <Column
            fullWidth={true}
            fullHeight={true}
            justifyContent='space-between'
          >
            {quoteOptions.length > 0 ? (
              <>
                <QuoteInfo
                  onSelectQuoteOption={onSelectQuoteOption}
                  selectedQuoteOptionId={selectedQuoteOptionId}
                  fromToken={fromToken}
                  toToken={toToken}
                  swapFees={swapFees}
                  isBridge={isBridge}
                  toAccount={toAccount}
                  onChangeRecipient={onChangeRecipient}
                  slippageTolerance={slippageTolerance}
                  onChangeSlippageTolerance={onChangeSlippageTolerance}
                  quoteOptions={quoteOptions}
                />
                <VerticalSpace space='12px' />
                {/* TODO: Swap and Send is currently unavailable
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
                /> */}
              </>
            ) : (
              <VerticalSpace space='2px' />
            )}
            <Column fullWidth={true}>
              {swapValidationError === 'providerNotSupported' && (
                <AlertMessage type='info'>
                  <AlertMessageWrapper
                    width='unset'
                    justifyContent='flex-start'
                    gap='4px'
                  >
                    {getLocale('braveWalletProviderNotSupported').replace(
                      '$1',
                      SwapProviderNameMapping[selectedProvider]
                    )}
                    <div>
                      <AlertMessageButton
                        kind='plain-faint'
                        size='tiny'
                        onClick={() => setShowSwapProviders(true)}
                      >
                        {getLocale('braveWalletChangeProvider')}
                      </AlertMessageButton>
                    </div>
                  </AlertMessageWrapper>
                </AlertMessage>
              )}
              <ReviewButtonRow width='100%'>
                <LeoSquaredButton
                  onClick={onSubmit}
                  size='large'
                  isDisabled={isSubmitButtonDisabled}
                  isLoading={isFetchingQuote || isSubmittingSwap}
                >
                  {submitButtonText}
                </LeoSquaredButton>
              </ReviewButtonRow>
            </Column>
          </Column>
        </ToAsset>
      </WalletPageWrapper>
      {selectingFromOrTo && (
        <SelectTokenModal
          ref={selectTokenModalRef}
          onClose={() => setSelectingFromOrTo(undefined)}
          onSelectAsset={
            selectingFromOrTo === 'from' ? onSelectFromToken : onSelectToToken
          }
          selectingFromOrTo={selectingFromOrTo}
          selectedFromToken={fromToken}
          selectedToToken={toToken}
          selectedNetwork={
            !isBridge && selectingFromOrTo === 'to' ? fromNetwork : undefined
          }
          modalType={isBridge ? 'bridge' : 'swap'}
          selectedSendOption='#token'
        />
      )}
      {showPrivacyModal && (
        <PrivacyModal
          ref={privacyModalRef}
          onClose={() => setShowPrivacyModal(false)}
        />
      )}
      {!isPanel && showSwapProviders && (
        <PopupModal
          title=''
          onClose={() => setShowSwapProviders(false)}
          width='560px'
          showDivider={false}
          height='unset'
        >
          <SwapProviders
            onChangeSwapProvider={handleOnChangeSwapProvider}
            selectedProvider={selectedProvider}
            availableProvidersForSwap={availableProvidersForSwap}
          />
        </PopupModal>
      )}
      {isPanel && (
        <BottomSheet
          onClose={() => setShowSwapProviders(false)}
          isOpen={showSwapProviders}
        >
          <SwapProviders
            onChangeSwapProvider={handleOnChangeSwapProvider}
            selectedProvider={selectedProvider}
            availableProvidersForSwap={availableProvidersForSwap}
          />
        </BottomSheet>
      )}
    </>
  )
}
