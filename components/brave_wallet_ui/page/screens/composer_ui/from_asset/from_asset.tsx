// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Utils
import {
  formatTokenBalanceWithSymbol,
  getBalance,
  getPercentAmount
} from '../../../../utils/balance-utils'
import { getLocale } from '../../../../../common/locale'
import {
  computeFiatAmount,
  getPriceIdForToken
} from '../../../../utils/pricing-utils'
import Amount from '../../../../utils/amount'

// Hooks
import {
  useOnClickOutside //
} from '../../../../common/hooks/useOnClickOutside'

// Queries
import {
  useGetBitcoinBalancesQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetTokenSpotPricesQuery
} from '../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s //
} from '../../../../common/slices/constants'
import {
  TokenBalancesRegistry //
} from '../../../../common/slices/entities/token-balance.entity'

// Types
import { BraveWallet } from '../../../../constants/types'

// Components
import { SelectButton } from '../select_button/select_button'
import {
  LoadingSkeleton //
} from '../../../../components/shared/loading-skeleton/index'
import {
  BalanceDetailsModal //
} from '../../../../components/desktop/popup-modals/balance_details_modal/balance_details_modal'
import {
  ShieldedLabel //
} from '../../../../components/shared/shielded_label/shielded_label'

// Styled Components
import {
  Wrapper,
  BalanceText,
  FiatText,
  FromText,
  NetworkText,
  AccountNameAndPresetsRow,
  NetworkAndFiatRow,
  SelectTokenAndInputRow,
  InfoIcon,
  AccountNameAndBalanceRow
} from './from_asset.style'
import { AmountInput, PresetButton } from '../shared_composer.style'
import {
  Column,
  Row,
  HorizontalSpace
} from '../../../../components/shared/style'

interface Props {
  onClickSelectToken: () => void
  onInputChange: (value: string, maxValue: boolean) => void
  inputValue: string
  hasInputError: boolean
  token: BraveWallet.BlockchainToken | undefined
  network: BraveWallet.NetworkInfo | undefined
  account: BraveWallet.AccountInfo | undefined
  tokenBalancesRegistry: TokenBalancesRegistry | undefined | null
  isLoadingBalances: boolean
}

export const FromAsset = (props: Props) => {
  const {
    token,
    onClickSelectToken,
    onInputChange: onChange,
    hasInputError,
    inputValue,
    network,
    tokenBalancesRegistry,
    isLoadingBalances,
    account
  } = props

  // State
  const [showBalanceDetailsModal, setShowBalanceDetailsModal] =
    React.useState<boolean>(false)

  // Refs
  const balanceDetailsRef = React.useRef<HTMLDivElement>(null)

  // Hooks
  useOnClickOutside(
    balanceDetailsRef,
    () => setShowBalanceDetailsModal(false),
    showBalanceDetailsModal
  )

  // Queries
  const { data: bitcoinBalances, isLoading: isLoadingBitcoinBalances } =
    useGetBitcoinBalancesQuery(
      token && token?.coin === BraveWallet.CoinType.BTC && account?.accountId
        ? account.accountId
        : skipToken
    )

  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const { data: spotPriceRegistry, isLoading: isLoadingSpotPrices } =
    useGetTokenSpotPricesQuery(
      token &&
        !token.isNft &&
        !token.isErc721 &&
        !token.isErc1155 &&
        defaultFiatCurrency
        ? {
            ids: [getPriceIdForToken(token)],
            toCurrency: defaultFiatCurrency
          }
        : skipToken,
      querySubscriptionOptions60s
    )

  // methods
  const setPresetAmountValue = React.useCallback(
    (percent: number) => {
      if (!token || !account) {
        return
      }

      onChange(
        getPercentAmount(
          token,
          account.accountId,
          percent,
          tokenBalancesRegistry
        ),
        percent === 1
      )
    },
    [token, account, tokenBalancesRegistry, onChange]
  )

  const onInputChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      onChange(event.target.value, false)
    },
    [onChange]
  )

  const tokenBalance = React.useMemo(() => {
    if (!account || !token || !tokenBalancesRegistry) {
      return ''
    }
    if (token.coin === BraveWallet.CoinType.BTC && bitcoinBalances) {
      return bitcoinBalances.availableBalance
    }
    return getBalance(account.accountId, token, tokenBalancesRegistry)
  }, [account, token, tokenBalancesRegistry, bitcoinBalances])

  const hasPendingBalance = !new Amount(
    bitcoinBalances?.pendingBalance ?? '0'
  ).isZero()

  const accountNameAndBalance = React.useMemo(() => {
    if (!token || !account) {
      return (
        <FromText
          textSize='14px'
          isBold={false}
        >
          {getLocale('braveWalletFrom')}
        </FromText>
      )
    }
    if (token.isNft) {
      return (
        <FromText
          textSize='14px'
          isBold={false}
        >
          {account.name}
        </FromText>
      )
    }
    return (
      <AccountNameAndBalanceRow
        width='unset'
        justifyContent='flex-start'
      >
        <FromText
          textSize='14px'
          isBold={false}
        >
          {account.name}:
        </FromText>
        <AccountNameAndBalanceRow
          width='unset'
          justifyContent='flex-start'
        >
          {isLoadingBalances || isLoadingBitcoinBalances ? (
            <LoadingSkeleton
              height={20}
              width={60}
            />
          ) : (
            <>
              <BalanceText
                textSize='14px'
                isBold={true}
                textColor='primary'
              >
                {formatTokenBalanceWithSymbol(
                  tokenBalance,
                  token.decimals,
                  token.symbol,
                  6
                )}
              </BalanceText>
              {token.isShielded && <ShieldedLabel />}
              {token.coin === BraveWallet.CoinType.BTC && hasPendingBalance && (
                <>
                  <BalanceText
                    textSize='14px'
                    isBold={true}
                    textColor='primary'
                  >
                    {`(${getLocale('braveWalletAvailable')})`}
                  </BalanceText>
                  <div>
                    <Button
                      kind='plain'
                      size='tiny'
                      onClick={() => setShowBalanceDetailsModal(true)}
                    >
                      <Row>
                        <InfoIcon />
                        {getLocale('braveWalletDetails')}
                      </Row>
                    </Button>
                  </div>
                </>
              )}
            </>
          )}
        </AccountNameAndBalanceRow>
      </AccountNameAndBalanceRow>
    )
  }, [
    token,
    tokenBalance,
    account,
    isLoadingBalances,
    isLoadingBitcoinBalances,
    hasPendingBalance
  ])

  const fiatValue = React.useMemo(() => {
    if (!token || token.isNft) {
      return ''
    }

    return computeFiatAmount({
      spotPriceRegistry,
      value: new Amount(inputValue !== '' ? inputValue : '0')
        .multiplyByDecimals(token.decimals)
        .toHex(),
      token: token
    }).formatAsFiat(defaultFiatCurrency)
  }, [spotPriceRegistry, token, inputValue, defaultFiatCurrency])

  // render
  return (
    <Wrapper fullWidth={true}>
      <Column
        fullWidth={true}
        justifyContent='space-between'
        alignItems='center'
        padding='0px 0px 32px 0px'
      >
        <AccountNameAndPresetsRow
          width='100%'
          alignItems='center'
          justifyContent='space-between'
          marginBottom={10}
        >
          {accountNameAndBalance}
          {token && !token.isNft && account !== undefined && (
            <Row width='unset'>
              <PresetButton onClick={() => setPresetAmountValue(0.5)}>
                {getLocale('braveWalletSendHalf')}
              </PresetButton>
              <HorizontalSpace space='8px' />
              <PresetButton onClick={() => setPresetAmountValue(1)}>
                {getLocale('braveWalletSendMax')}
              </PresetButton>
            </Row>
          )}
        </AccountNameAndPresetsRow>
        <SelectTokenAndInputRow
          width='100%'
          alignItems='center'
          justifyContent='space-between'
          marginBottom={10}
        >
          <Row width='unset'>
            <SelectButton
              onClick={onClickSelectToken}
              token={token}
              selectedSendOption={token?.isNft ? '#nft' : '#token'}
              placeholderText={getLocale('braveWalletChooseAsset')}
            />
          </Row>
          {!token?.isNft && (
            <AmountInput
              placeholder='0.0'
              type='number'
              spellCheck={false}
              onChange={onInputChange}
              value={inputValue}
              hasError={hasInputError}
              autoFocus={true}
            />
          )}
        </SelectTokenAndInputRow>
        <NetworkAndFiatRow
          width='100%'
          alignItems='center'
          justifyContent='space-between'
        >
          {network && token && (
            <NetworkText
              textSize='14px'
              isBold={false}
              textColor='secondary'
            >
              {getLocale('braveWalletPortfolioAssetNetworkDescription')
                .replace('$1', '')
                .replace('$2', network.chainName)}
            </NetworkText>
          )}
          {token && !token.isNft && (
            <Row width='unset'>
              {isLoadingSpotPrices ? (
                <LoadingSkeleton
                  height={18}
                  width={60}
                />
              ) : (
                <FiatText
                  textSize='14px'
                  isBold={false}
                >
                  {fiatValue}
                </FiatText>
              )}
            </Row>
          )}
        </NetworkAndFiatRow>
      </Column>
      {showBalanceDetailsModal && token && (
        <BalanceDetailsModal
          ref={balanceDetailsRef}
          onClose={() => setShowBalanceDetailsModal(false)}
          token={token}
          isLoadingBalances={isLoadingBitcoinBalances}
          balances={bitcoinBalances}
        />
      )}
    </Wrapper>
  )
}
