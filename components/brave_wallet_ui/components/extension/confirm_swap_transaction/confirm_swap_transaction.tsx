// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Tooltip from '@brave/leo/react/tooltip'
import Icon from '@brave/leo/react/icon'

// Queries
import {
  useGetActiveOriginQuery,
  useGetNetworkQuery,
  useGetTokenInfoQuery,
} from '../../../common/slices/api.slice'

// Constants
import { BraveWallet } from '../../../constants/types'
import {
  NATIVE_EVM_ASSET_CONTRACT_ADDRESS,
  UNKNOWN_TOKEN_COINGECKO_ID,
} from '../../../common/constants/magics'

// Utils
import { getLocale } from '../../../../common/locale'
import { isBridgeTransaction } from '../../../utils/tx-utils'
import { reduceAddress } from '../../../utils/reduce-address'
import { getAddressLabel } from '../../../utils/account-utils'

// Hooks
import {
  usePendingTransactions, //
} from '../../../common/hooks/use-pending-transaction'
import {
  useSwapTransactionParser, //
} from '../../../common/hooks/use-swap-tx-parser'
import { useExplorer } from '../../../common/hooks/explorer'

// Components
import { OriginInfoCard } from '../origin_info_card/origin_info_card'
import {
  ConfirmationFooterActions, //
} from '../confirmation_footer_actions/confirmation_footer_actions'
import {
  ConfirmationNetworkFee, //
} from '../confirmation_network_fee/confirmation_network_fee'
import {
  ConfirmationTokenInfo, //
} from '../confirmation_token_info/confirmation_token_info'
import { ConfirmationHeader } from '../confirmation_header/confirmation_header'
import { LoadingPanel } from '../loading_panel/loading_panel'
import { BottomSheet } from '../../shared/bottom_sheet/bottom_sheet'
import {
  PendingTransactionDetails, //
} from '../pending_transaction_details/pending_transaction_details'
import {
  AdvancedTransactionSettings, //
} from '../advanced_transaction_settings/advanced_transaction_settings'
import { EditNetworkFee } from '../edit_network_fee/edit_network_fee'
import {
  ConfirmRejectButtons, //
} from '../confirm_reject_buttons/confirm_reject_buttons'
import {
  ConfirmationError, //
} from '../confirmation_error/confirmation_error'

// Styled Components
import {
  StyledWrapper,
  InfoBox,
  Card,
  ArrowIconContainer,
} from './confirm_swap_transaction.style'
import { Column, Row, VerticalDivider } from '../../shared/style'
import {
  ConfirmationButtonLink,
  ConfirmationInfoLabel,
  ScrollableColumn,
} from '../shared-panel-styles'

const isNativeToken = (token: BraveWallet.BlockchainToken) =>
  token.contractAddress === ''
  || token.contractAddress.toLowerCase() === NATIVE_EVM_ASSET_CONTRACT_ADDRESS

const makeToken = (
  token?: BraveWallet.BlockchainToken,
  network?: BraveWallet.NetworkInfo,
  symbol?: string,
  decimals?: number,
) => {
  if (!token || !network) {
    return undefined
  }

  if (token.coingeckoId !== UNKNOWN_TOKEN_COINGECKO_ID) {
    return token
  }

  if (isNativeToken(token)) {
    return {
      ...token,
      decimals: network.decimals,
      symbol: network.symbol,
    }
  }

  if (!symbol || !decimals) {
    return undefined
  }

  return {
    ...token,
    symbol,
    decimals,
  }
}

export function ConfirmSwapTransaction() {
  // State
  const [showEditNetworkFee, setShowEditNetworkFee] =
    React.useState<boolean>(false)
  const [showAdvancedTransactionSettings, setShowAdvancedTransactionSettings] =
    React.useState<boolean>(false)
  const [showTransactionDetails, setShowTransactionDetails] =
    React.useState<boolean>(false)

  // Queries
  const { data: activeOrigin = { eTldPlusOne: '', originSpec: '' } } =
    useGetActiveOriginQuery()

  // Hooks
  const {
    transactionDetails,
    selectedPendingTransaction,
    queueNextTransaction,
    queuePreviousTransaction,
    transactionsQueueLength,
    rejectAllTransactions,
    onConfirm,
    onReject,
    transactionsNetwork,
    gasFee,
    updateUnapprovedTransactionNonce,
    fromAccount,
    canEditNetworkFee,
    isEthereumTransaction,
    isConfirmButtonDisabled,
    insufficientFundsError,
    insufficientFundsForGasError,
  } = usePendingTransactions()

  const onClickViewOnBlockExplorer = useExplorer(transactionsNetwork)

  // Computed
  const originInfo = selectedPendingTransaction?.originInfo ?? activeOrigin
  const isBridgeTx = selectedPendingTransaction
    ? isBridgeTransaction(selectedPendingTransaction)
    : false
  const toChainId = selectedPendingTransaction?.swapInfoDeprecated?.toChainId
  const toCoin = selectedPendingTransaction?.swapInfoDeprecated?.toCoin

  // Queries
  const { buyToken, sellToken, buyAmountWei, sellAmountWei } =
    useSwapTransactionParser(selectedPendingTransaction)

  const { data: sellAssetNetwork } = useGetNetworkQuery(sellToken ?? skipToken)

  const { data: buyAssetNetwork } = useGetNetworkQuery(
    isBridgeTx && toChainId && toCoin
      ? { chainId: toChainId, coin: toCoin }
      : (buyToken ?? skipToken),
  )

  const { data: sellTokenInfo } = useGetTokenInfoQuery(
    sellToken
      && sellToken.coingeckoId === UNKNOWN_TOKEN_COINGECKO_ID
      && !isNativeToken(sellToken)
      ? {
          contractAddress: sellToken.contractAddress,
          chainId: sellToken.chainId,
          coin: sellToken.coin,
        }
      : skipToken,
  )

  const { data: buyTokenInfo } = useGetTokenInfoQuery(
    buyToken
      && buyToken.coingeckoId === UNKNOWN_TOKEN_COINGECKO_ID
      && !isNativeToken(buyToken)
      ? {
          contractAddress: buyToken.contractAddress,
          chainId: buyToken.chainId,
          coin: buyToken.coin,
        }
      : skipToken,
  )

  // Memos
  const buyTokenResult = React.useMemo(
    () =>
      makeToken(
        buyToken,
        buyAssetNetwork,
        buyTokenInfo?.symbol,
        buyTokenInfo?.decimals,
      ),
    [buyToken, buyAssetNetwork, buyTokenInfo],
  )
  const sellTokenResult = React.useMemo(
    () =>
      makeToken(
        sellToken,
        sellAssetNetwork,
        sellTokenInfo?.symbol,
        sellTokenInfo?.decimals,
      ),
    [sellToken, sellAssetNetwork, sellTokenInfo],
  )

  if (!selectedPendingTransaction || !transactionDetails) {
    return <LoadingPanel />
  }

  return (
    <>
      <StyledWrapper
        width='100%'
        height='100%'
        justifyContent='space-between'
      >
        {/* Header */}
        <ConfirmationHeader
          title={
            isBridgeTx
              ? getLocale('braveWalletConfirmBridge')
              : getLocale('braveWalletConfirmSwap')
          }
          transactionsQueueLength={transactionsQueueLength}
          queueNextTransaction={queueNextTransaction}
          queuePreviousTransaction={queuePreviousTransaction}
          rejectAllTransactions={rejectAllTransactions}
        />
        <ScrollableColumn
          width='100%'
          height='100%'
          justifyContent='flex-start'
        >
          {originInfo && (
            <Row
              justifyContent='flex-start'
              padding='0px 0px 14px 0px'
            >
              <OriginInfoCard
                origin={originInfo}
                noBackground={true}
                provider={getAddressLabel(transactionDetails.recipient) ?? ''}
              />
            </Row>
          )}
          <Column
            width='100%'
            height='100%'
            padding='0px 16px'
            gap='8px'
            justifyContent='flex-start'
          >
            <InfoBox width='100%'>
              {/* Swap details */}
              <Card
                width='100%'
                padding='16px'
              >
                {/* Sell token */}
                <ConfirmationTokenInfo
                  token={sellTokenResult}
                  label='spend'
                  amount={
                    !sellAmountWei.isUndefined()
                      ? sellAmountWei.format()
                      : undefined
                  }
                  network={sellAssetNetwork}
                  account={fromAccount}
                />

                {/* Divider */}
                <Row
                  justifyContent='space-between'
                  gap='24px'
                >
                  <ArrowIconContainer>
                    <Icon name='arrow-down' />
                  </ArrowIconContainer>
                  <VerticalDivider />
                </Row>

                {/* Buy token */}
                <ConfirmationTokenInfo
                  token={buyTokenResult}
                  label={isBridgeTx ? 'bridge' : 'receive'}
                  amount={
                    !buyAmountWei.isUndefined()
                      ? buyAmountWei.format()
                      : undefined
                  }
                  network={buyAssetNetwork}
                  receiveAddress={
                    selectedPendingTransaction.swapInfoDeprecated?.receiver
                  }
                />
              </Card>

              {/* Network fee details */}
              <Column
                width='100%'
                padding='16px'
                gap='8px'
              >
                <ConfirmationNetworkFee
                  transactionsNetwork={transactionsNetwork}
                  gasFee={gasFee}
                  transactionDetails={transactionDetails}
                  onClickEditNetworkFee={
                    canEditNetworkFee
                      ? () => setShowEditNetworkFee(true)
                      : undefined
                  }
                />
                <VerticalDivider />
                <Row justifyContent='space-between'>
                  <ConfirmationInfoLabel textColor='secondary'>
                    {getLocale('braveWalletNFTDetailContractAddress')}
                  </ConfirmationInfoLabel>
                  <Row
                    width='unset'
                    gap='4px'
                  >
                    <Tooltip text={transactionDetails.recipient}>
                      <ConfirmationButtonLink
                        onClick={onClickViewOnBlockExplorer(
                          'contract',
                          transactionDetails.recipient,
                        )}
                      >
                        {reduceAddress(transactionDetails.recipient)}
                        <Icon name='arrow-diagonal-up-right' />
                      </ConfirmationButtonLink>
                    </Tooltip>
                  </Row>
                </Row>
              </Column>

              {/* Transaction errors */}
              <ConfirmationError
                insufficientFundsError={insufficientFundsError}
                insufficientFundsForGasError={insufficientFundsForGasError}
                transactionDetails={transactionDetails}
                transactionsNetwork={transactionsNetwork}
                account={fromAccount}
              />
            </InfoBox>
            <ConfirmationFooterActions
              onClickAdvancedSettings={
                isEthereumTransaction
                  ? () => setShowAdvancedTransactionSettings(true)
                  : undefined
              }
              onClickDetails={() => {
                setShowTransactionDetails(true)
              }}
            />
          </Column>
        </ScrollableColumn>

        {/* Confirm and reject buttons */}
        <ConfirmRejectButtons
          onConfirm={onConfirm}
          onReject={onReject}
          isConfirmButtonDisabled={isConfirmButtonDisabled}
        />
      </StyledWrapper>
      <BottomSheet
        isOpen={showTransactionDetails}
        title={getLocale('braveWalletDetails')}
        onClose={() => setShowTransactionDetails(false)}
      >
        <PendingTransactionDetails
          transactionInfo={selectedPendingTransaction}
          instructions={transactionDetails.instructions}
        />
      </BottomSheet>
      <BottomSheet
        isOpen={showAdvancedTransactionSettings}
        title={getLocale('braveWalletAdvancedTransactionSettings')}
        onClose={() => setShowAdvancedTransactionSettings(false)}
      >
        <AdvancedTransactionSettings
          onCancel={() => setShowAdvancedTransactionSettings(false)}
          nonce={transactionDetails.nonce}
          onSave={(nonce: string) =>
            updateUnapprovedTransactionNonce({
              chainId: selectedPendingTransaction.chainId,
              txMetaId: selectedPendingTransaction.id,
              nonce: nonce,
            })
          }
        />
      </BottomSheet>
      <EditNetworkFee
        isOpen={showEditNetworkFee}
        onCancel={() => setShowEditNetworkFee(false)}
      />
    </>
  )
}
