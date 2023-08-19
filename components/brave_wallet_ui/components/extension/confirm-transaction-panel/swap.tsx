// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Utils
import { WalletSelectors } from '../../../common/selectors'
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'
import { openBlockExplorerURL } from '../../../utils/block-explorer-utils'

// Styled components
import {
  HeaderTitle,
  ExchangeRate,
  SwapDetails,
  SwapDetailsDivider,
  SwapAssetContainer,
  SwapAssetAddress,
  SwapAssetTitle,
  AddressOrb,
  AccountNameText,
  SwapAssetHeader,
  SwapDetailsArrow,
  SwapDetailsArrowContainer,
  SwapAssetDetailsContainer,
  AssetIcon,
  SwapAmountColumn,
  NetworkDescriptionText,
  Spacer,
  SwapAssetAmountSymbol,
} from './swap.style'
import { EditButton, NetworkText, StyledWrapper, TopRow } from './style'
import { CreateNetworkIcon } from '../../shared/create-network-icon/index'
import { LoadingSkeleton } from '../../shared/loading-skeleton/index'
import { withPlaceholderIcon } from '../../shared/create-placeholder-icon/index'
import { IconsWrapper as SwapAssetIconWrapper, NetworkIconWrapper } from '../../shared/style'
import { NftIcon } from '../../shared/nft-icon/nft-icon'
import { Origin } from './common/origin'
import { EditPendingTransactionGas } from './common/gas'

// Components
import { TransactionQueueStep } from './common/queue'
import { Footer } from './common/footer'
import AdvancedTransactionSettings from '../advanced-transaction-settings'
import {
  PendingTransactionNetworkFeeAndSettings //
} from '../pending-transaction-network-fee/pending-transaction-network-fee'

// Types
import { BraveWallet } from '../../../constants/types'
import { UNKNOWN_TOKEN_COINGECKO_ID } from '../../../common/constants/magics'

// Hooks
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'
import { useGetNetworkQuery } from '../../../common/slices/api.slice'
import {
  useUnsafeWalletSelector //
} from '../../../common/hooks/use-safe-selector'

export function ConfirmSwapTransaction () {
  // redux
  const activeOrigin = useUnsafeWalletSelector(WalletSelectors.activeOrigin)

  // state
  const [showAdvancedTransactionSettings, setShowAdvancedTransactionSettings] =
    React.useState<boolean>(false)
  const [isEditingGas, setIsEditingGas] = React.useState<boolean>(false)

  // hooks
  const {
    transactionDetails,
    fromOrb,
    toOrb,
    updateUnapprovedTransactionNonce,
    selectedPendingTransaction,
    onConfirm,
    onReject
  } = usePendingTransactions()

  // queries
  const { data: makerAssetNetwork } = useGetNetworkQuery(
    transactionDetails?.sellToken ?? skipToken
  )

  const { data: takerAssetNetwork } = useGetNetworkQuery(
    transactionDetails?.buyToken ?? skipToken
  )

  // computed
  const originInfo = selectedPendingTransaction?.originInfo ?? activeOrigin

  // Methods
  const onToggleAdvancedTransactionSettings = () => {
    setShowAdvancedTransactionSettings(!showAdvancedTransactionSettings)
  }
  const onToggleEditGas = () => setIsEditingGas(!isEditingGas)

  // render
  if (
    showAdvancedTransactionSettings &&
    transactionDetails &&
    selectedPendingTransaction
  ) {
    return (
      <AdvancedTransactionSettings
        onCancel={onToggleAdvancedTransactionSettings}
        nonce={transactionDetails.nonce}
        chainId={selectedPendingTransaction.chainId}
        txMetaId={selectedPendingTransaction.id}
        updateUnapprovedTransactionNonce={updateUnapprovedTransactionNonce}
      />
    )
  }

  if (isEditingGas) {
    return <EditPendingTransactionGas onCancel={onToggleEditGas} />
  }

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText />
        <TransactionQueueStep />
      </TopRow>

      <HeaderTitle>{getLocale('braveWalletSwapReviewHeader')}</HeaderTitle>

      <Origin originInfo={originInfo} />

      {transactionDetails?.sellToken &&
        transactionDetails?.buyToken &&
        transactionDetails?.minBuyAmount &&
        transactionDetails?.sellAmount &&
        transactionDetails?.buyToken?.coingeckoId !==
          UNKNOWN_TOKEN_COINGECKO_ID &&
        transactionDetails?.sellToken?.coingeckoId !==
          UNKNOWN_TOKEN_COINGECKO_ID && (
          <ExchangeRate>
            1 {transactionDetails.sellToken.symbol} ={' '}
            {transactionDetails.minBuyAmount
              .div(transactionDetails.sellAmount)
              .format(6)}{' '}
            {transactionDetails.buyToken.symbol}
          </ExchangeRate>
        )}
      <SwapDetails>
        <SwapAsset
          type={'maker'}
          network={makerAssetNetwork}
          address={transactionDetails?.senderLabel}
          orb={fromOrb}
          expectAddress={true}
          asset={transactionDetails?.sellToken}
          amount={transactionDetails?.sellAmount}
        />

        <SwapDetailsDivider />
        <SwapDetailsArrowContainer>
          <SwapDetailsArrow />
        </SwapDetailsArrowContainer>

        <SwapAsset
          type={'taker'}
          network={takerAssetNetwork}
          address={transactionDetails?.recipientLabel}
          orb={toOrb}
          expectAddress={false} // set to true once Swap+Send is supported
          asset={transactionDetails?.buyToken}
          amount={transactionDetails?.minBuyAmount}
        />
      </SwapDetails>

      <PendingTransactionNetworkFeeAndSettings
        onToggleAdvancedTransactionSettings={
          onToggleAdvancedTransactionSettings
        }
        onToggleEditGas={onToggleEditGas}
      />

      <Footer
        onConfirm={onConfirm}
        onReject={onReject}
        rejectButtonType={'cancel'}
      />
    </StyledWrapper>
  )
}

interface SwapAssetProps {
  type: 'maker' | 'taker'
  network?: BraveWallet.NetworkInfo
  amount?: Amount
  asset?: BraveWallet.BlockchainToken
  address?: string
  orb?: string
  expectAddress?: boolean
}

function SwapAsset (props: SwapAssetProps) {
  const { type, address, orb, expectAddress, asset, amount, network } = props

  // Memos
  const AssetIconWithPlaceholder = React.useMemo(() => {
    return (
      asset &&
      withPlaceholderIcon(asset.isErc721 ? NftIcon : AssetIcon, {
        size: 'big',
        marginLeft: 0,
        marginRight: 8
      })
    )
  }, [asset])

  const networkDescription = React.useMemo(() => {
    if (network) {
      return getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', '')
        .replace('$2', network.chainName ?? '')
        .trim()
    }

    return ''
  }, [network])

  // computed
  const isUnknownAsset = asset?.coingeckoId === UNKNOWN_TOKEN_COINGECKO_ID

  return (
    <SwapAssetContainer top={type === 'maker'}>
      <SwapAssetHeader>
        <SwapAssetTitle>
          {type === 'maker'
            ? getLocale('braveWalletSwapReviewSpend')
            : getLocale('braveWalletSwapReviewReceive')}
        </SwapAssetTitle>
        {expectAddress && (
          <SwapAssetAddress>
            {address && orb ? (
              <AddressOrb orb={orb} />
            ) : (
              <LoadingSkeleton width={10} height={10} circle={true} />
            )}

            {address ? (
              <AccountNameText>{address}</AccountNameText>
            ) : (
              <LoadingSkeleton width={84} />
            )}
          </SwapAssetAddress>
        )}
      </SwapAssetHeader>

      <SwapAssetDetailsContainer>
        <SwapAssetIconWrapper>
          {!AssetIconWithPlaceholder || !asset || !asset.logo || !network ? (
            <LoadingSkeleton circle={true} width={40} height={40} />
          ) : (
            <>
              <AssetIconWithPlaceholder asset={asset} network={network} />
              {network && asset.contractAddress !== '' && (
                <NetworkIconWrapper>
                  <CreateNetworkIcon network={network} marginRight={0} />
                </NetworkIconWrapper>
              )}
            </>
          )}
        </SwapAssetIconWrapper>
        <SwapAmountColumn>
          {!networkDescription || !asset || !amount ? (
            <>
              <LoadingSkeleton width={200} height={18} />
              <Spacer />
              <LoadingSkeleton width={200} height={18} />
            </>
          ) : isUnknownAsset ? (
            <>
              <SwapAssetAmountSymbol>{asset.symbol}</SwapAssetAmountSymbol>
              <EditButton
                onClick={openBlockExplorerURL({
                  type: 'token',
                  network,
                  value: asset.contractAddress
                })}
              >
                {getLocale('braveWalletTransactionExplorer')}
              </EditButton>
            </>
          ) : (
            <>
              <SwapAssetAmountSymbol>{amount.formatAsAsset(6, asset.symbol)}</SwapAssetAmountSymbol>
              <NetworkDescriptionText>{networkDescription}</NetworkDescriptionText>
            </>
          )}
        </SwapAmountColumn>
      </SwapAssetDetailsContainer>
    </SwapAssetContainer>
  )
}
