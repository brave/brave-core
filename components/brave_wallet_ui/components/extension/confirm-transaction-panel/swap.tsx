// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { WalletSelectors } from '../../../common/selectors'
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'

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
  NetworkFeeAndSettingsContainer,
  NetworkFeeTitle,
  NetworkFeeContainer,
  NetworkFeeValue,
  Settings,
  SettingsIcon
} from './swap.style'
import { EditButton, NetworkText, StyledWrapper, TopRow } from './style'
import { CreateNetworkIcon, LoadingSkeleton, withPlaceholderIcon } from '../../shared'
import { IconsWrapper as SwapAssetIconWrapper, NetworkIconWrapper } from '../../shared/style'
import { NftIcon } from '../../shared/nft-icon/nft-icon'
import { Origin } from './common/origin'
import { EditPendingTransactionGas } from './common/gas'

// Components
import { TransactionQueueStep } from './common/queue'
import { Footer } from './common/footer'
import AdvancedTransactionSettings from '../advanced-transaction-settings'

// Types
import { BraveWallet } from '../../../constants/types'

// Hooks
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'
import { useGetNetworkQuery } from '../../../common/slices/api.slice'
import {
  useSafeWalletSelector,
  useUnsafeWalletSelector
} from '../../../common/hooks/use-safe-selector'

interface Props {
  onConfirm: () => void
  onReject: () => void
}

export function ConfirmSwapTransaction (props: Props) {
  const { onConfirm, onReject } = props

  // redux
  const defaultFiatCurrency = useSafeWalletSelector(
    WalletSelectors.defaultFiatCurrency
  )
  const activeOrigin = useUnsafeWalletSelector(WalletSelectors.activeOrigin)
  const transactionInfo = useUnsafeWalletSelector(
    WalletSelectors.selectedPendingTransaction
  )

  // state
  const [showAdvancedTransactionSettings, setShowAdvancedTransactionSettings] =
    React.useState<boolean>(false)
  const [isEditingGas, setIsEditingGas] = React.useState<boolean>(false)

  // hooks
  const {
    transactionDetails,
    transactionsNetwork,
    fromOrb,
    toOrb,
    updateUnapprovedTransactionNonce
  } = usePendingTransactions()

  // queries
  const { data: makerAssetNetwork } = useGetNetworkQuery(
    transactionDetails?.sellToken,
    {
      skip: !transactionDetails?.sellToken
    }
  )

  const { data: takerAssetNetwork } = useGetNetworkQuery(
    transactionDetails?.buyToken,
    {
      skip: !transactionDetails?.buyToken
    }
  )

  // computed
  const originInfo = transactionInfo?.originInfo ?? activeOrigin

  // Methods
  const onToggleAdvancedTransactionSettings = () => {
    setShowAdvancedTransactionSettings(!showAdvancedTransactionSettings)
  }
  const onToggleEditGas = () => setIsEditingGas(!isEditingGas)

  // render
  if (showAdvancedTransactionSettings && transactionDetails && transactionInfo) {
    return (
      <AdvancedTransactionSettings
        onCancel={onToggleAdvancedTransactionSettings}
        nonce={transactionDetails.nonce}
        txMetaId={transactionInfo.id}
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
        transactionDetails?.sellAmount && (
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

      <NetworkFeeAndSettingsContainer>
        <NetworkFeeContainer>
          <NetworkFeeTitle>Network fee</NetworkFeeTitle>
          <NetworkFeeValue>
            <CreateNetworkIcon network={transactionsNetwork} marginRight={0} />
            {transactionDetails?.gasFeeFiat ? (
              new Amount(transactionDetails.gasFeeFiat).formatAsFiat(
                defaultFiatCurrency
              )
            ) : (
              <LoadingSkeleton width={38} />
            )}
            <EditButton onClick={onToggleEditGas}>
              {getLocale('braveWalletAllowSpendEditButton')}
            </EditButton>
          </NetworkFeeValue>
        </NetworkFeeContainer>

        <Settings onClick={onToggleAdvancedTransactionSettings}>
          <SettingsIcon />
        </Settings>
      </NetworkFeeAndSettingsContainer>

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
