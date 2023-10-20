// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useHistory } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import {
  AccountPageTabs,
  BraveWallet,
  DefaultCurrencies
} from '../../../constants/types'

// Hooks
import { useExplorer } from '../../../common/hooks/explorer'
import {
  useOnClickOutside
} from '../../../common/hooks/useOnClickOutside'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import Amount from '../../../utils/amount'
import { getPriceIdForToken } from '../../../utils/api-utils'
import { computeFiatAmount } from '../../../utils/pricing-utils'
import { makeAccountRoute } from '../../../utils/routes-utils'
import {
  getIsRewardsAccount
} from '../../../utils/rewards_utils'
import {
  externalWalletProviderFromString
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'
import { getLocale } from '../../../../common/locale'

// Components
import WithHideBalancePlaceholder from '../with-hide-balance-placeholder'
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'
import {
  PortfolioAccountMenu
} from '../wallet-menus/portfolio-account-menu'
import { RewardsMenu } from '../wallet-menus/rewards_menu'

// Styled Components
import {
  CreateAccountIcon
} from '../../shared/create-account-icon/create-account-icon'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetTokenSpotPricesQuery
} from '../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s
} from '../../../common/slices/constants'

// Styled Components
import {
  StyledWrapper,
  AssetBalanceText,
  AccountNameButton,
  AccountAddressButton,
  AccountAndAddress,
  BalanceColumn,
  FiatBalanceText,
  NameAndIcon,
  AccountMenuWrapper,
  AccountMenuButton,
  AccountMenuIcon,
  RightSide,
  CopyIcon,
  AddressAndButtonRow
} from './style'
import {
  BraveRewardsIndicator,
  VerticalSpacer
} from '../../shared/style'

interface Props {
  account: BraveWallet.AccountInfo
  defaultCurrencies: DefaultCurrencies
  asset: BraveWallet.BlockchainToken
  assetBalance: string
  selectedNetwork?: BraveWallet.NetworkInfo | null
  hideBalances?: boolean
  isSellSupported: boolean
  showSellModal: () => void
}

export const PortfolioAccountItem = (props: Props) => {
  const {
    asset,
    assetBalance,
    account,
    selectedNetwork,
    defaultCurrencies,
    hideBalances,
    isSellSupported,
    showSellModal
  } = props

  // Routing
  const history = useHistory()

  // Hooks
  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  // State
  const [showAccountMenu, setShowAccountMenu] = React.useState<boolean>(false)

  // Refs
  const accountMenuRef = React.useRef<HTMLDivElement>(null)

  // Memos & Computed
  const isRewardsAccount = getIsRewardsAccount(account.accountId)

  const externalProvider =
    isRewardsAccount
      ? externalWalletProviderFromString(account.accountId.uniqueKey)
      : null

  const formattedAssetBalance: string = React.useMemo(() => {
    return new Amount(assetBalance)
      .divideByDecimals(asset.decimals)
      .format(6, true)
  }, [assetBalance, asset.decimals])

  const tokenPriceIds = React.useMemo(
    () => [getPriceIdForToken(asset)],
    [asset]
  )

  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    defaultFiatCurrency && tokenPriceIds.length
      ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s
  )

  const fiatBalance: Amount = React.useMemo(() => {
    return computeFiatAmount({
      spotPriceRegistry,
      value: assetBalance,
      token: asset
    })
  }, [spotPriceRegistry, assetBalance, asset])

  const isAssetsBalanceZero = React.useMemo(() => {
    return new Amount(assetBalance).isZero()
  }, [assetBalance])

  // Methods
  const onSelectAccount = React.useCallback(() => {
    history.push(makeAccountRoute(account, AccountPageTabs.AccountAssetsSub))
  }, [history, account])

  const onHideAccountMenu = React.useCallback(() => {
    setShowAccountMenu(false)
  }, [])

  // Hooks
  useOnClickOutside(
    accountMenuRef,
    onHideAccountMenu,
    showAccountMenu
  )

  return (
    <StyledWrapper>
      <NameAndIcon>
        <CreateAccountIcon
          size='big'
          marginRight={12}
          account={account}
          externalProvider={externalProvider}
        />
        <AccountAndAddress>
          <AccountNameButton onClick={onSelectAccount}>{account.name}</AccountNameButton>
          {isRewardsAccount &&
            <>
              <VerticalSpacer space='6px' />
              <BraveRewardsIndicator>
                {getLocale('braveWalletBraveRewardsTitle')}
              </BraveRewardsIndicator>
            </>
          }
          {account.address && !isRewardsAccount &&
            <AddressAndButtonRow>
              <AccountAddressButton onClick={onSelectAccount}>
                {reduceAddress(account.address)}
              </AccountAddressButton>
              <CopyTooltip text={account.address}>
                <CopyIcon />
              </CopyTooltip>
            </AddressAndButtonRow>
          }
        </AccountAndAddress>

      </NameAndIcon>
      <RightSide>
        <BalanceColumn>
          <WithHideBalancePlaceholder
            size='small'
            hideBalances={hideBalances ?? false}
          >
            <AssetBalanceText>
              {`${formattedAssetBalance} ${asset.symbol}`}
            </AssetBalanceText>
            <FiatBalanceText>
              {fiatBalance.formatAsFiat(defaultCurrencies.fiat)}
            </FiatBalanceText>
          </WithHideBalancePlaceholder>
        </BalanceColumn>
        <AccountMenuWrapper
          ref={accountMenuRef}
        >
          <AccountMenuButton
            onClick={() => setShowAccountMenu(prev => !prev)}
          >
            <AccountMenuIcon />
          </AccountMenuButton>
          {showAccountMenu &&
            <>
              {isRewardsAccount ? (
                <RewardsMenu />
              ) : (
                <PortfolioAccountMenu
                  onClickViewOnExplorer={
                    onClickViewOnBlockExplorer('address', account.address)
                  }
                  onClickSell={
                    isSellSupported && !isAssetsBalanceZero
                      ? showSellModal
                      : undefined
                  }
                />
              )}
            </>
          }
        </AccountMenuWrapper>
      </RightSide>
    </StyledWrapper>
  )
}

export default PortfolioAccountItem
