// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useHistory } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { AccountPageTabs, BraveWallet } from '../../../constants/types'

// Hooks
import { useExplorer } from '../../../common/hooks/explorer'
import { useOnClickOutside } from '../../../common/hooks/useOnClickOutside'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import Amount from '../../../utils/amount'
import { getPriceIdForToken } from '../../../utils/api-utils'
import { computeFiatAmount } from '../../../utils/pricing-utils'
import { makeAccountRoute } from '../../../utils/routes-utils'
import { getIsRewardsAccount } from '../../../utils/rewards_utils'
import {
  externalWalletProviderFromString //
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'
import { getLocale } from '../../../../common/locale'

// Components
import WithHideBalancePlaceholder from '../with-hide-balance-placeholder'
import { PortfolioAccountMenu } from '../wallet-menus/portfolio-account-menu'
import { RewardsMenu } from '../wallet-menus/rewards_menu'
import { PopupModal } from '../popup-modals/index'
import { DepositModal } from '../popup-modals/account-settings-modal/account-settings-modal'

// Styled Components
import {
  CreateAccountIcon //
} from '../../shared/create-account-icon/create-account-icon'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetTokenSpotPricesQuery
} from '../../../common/slices/api.slice'
import { querySubscriptionOptions60s } from '../../../common/slices/constants'

// Styled Components
import {
  StyledWrapper,
  AccountMenuWrapper,
  AccountMenuButton,
  AccountMenuIcon,
  AccountButton
} from './style'
import {
  BraveRewardsIndicator,
  VerticalSpacer,
  Text,
  Row,
  Column,
  VerticalDivider
} from '../../shared/style'

interface Props {
  account: BraveWallet.AccountInfo
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
  const [showDepositModal, setShowDepositModal] = React.useState<boolean>(false)

  // Refs
  const accountMenuRef = React.useRef<HTMLDivElement>(null)
  const depositModalRef = React.useRef<HTMLDivElement>(null)

  // Memos & Computed
  const isRewardsAccount = getIsRewardsAccount(account.accountId)

  const externalProvider = isRewardsAccount
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

  // Queries
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()

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

  const blockExplorerSupported = !!account.address

  // Methods
  const onSelectAccount = React.useCallback(() => {
    history.push(makeAccountRoute(account, AccountPageTabs.AccountAssetsSub))
  }, [history, account])

  const onViewAccountOnBlockExplorer = React.useCallback(
    () => onClickViewOnBlockExplorer('address', account.address)(),
    [account.address, onClickViewOnBlockExplorer]
  )

  const onHideAccountMenu = React.useCallback(() => {
    setShowAccountMenu(false)
  }, [])

  const onShowDepositModal = () => {
    setShowDepositModal(true)
    setShowAccountMenu(false)
  }

  // Hooks
  useOnClickOutside(accountMenuRef, onHideAccountMenu, showAccountMenu)
  useOnClickOutside(
    depositModalRef,
    () => setShowDepositModal(false),
    showDepositModal
  )

  return (
    <>
      <StyledWrapper isRewardsAccount={isRewardsAccount}>
        <AccountButton
          onClick={onSelectAccount}
          disabled={isRewardsAccount}
        >
          <Row width='unset'>
            <CreateAccountIcon
              size='huge'
              marginRight={12}
              account={account}
              externalProvider={externalProvider}
            />
            <Column alignItems='flex-start'>
              <Text
                textSize='14px'
                isBold={true}
                textColor='primary'
                textAlign='left'
              >
                {account.name}
              </Text>
              {isRewardsAccount && (
                <>
                  <VerticalSpacer space='6px' />
                  <BraveRewardsIndicator>
                    {getLocale('braveWalletBraveRewardsTitle')}
                  </BraveRewardsIndicator>
                </>
              )}
              {account.address && !isRewardsAccount && (
                <Text
                  textSize='12px'
                  isBold={false}
                  textColor='primary'
                  textAlign='left'
                >
                  {reduceAddress(account.address)}
                </Text>
              )}
            </Column>
          </Row>
          <Column
            alignItems='flex-end'
            margin='0px 12px 0px 0px'
          >
            <WithHideBalancePlaceholder
              size='small'
              hideBalances={hideBalances ?? false}
            >
              <Text
                textSize='14px'
                isBold={true}
                textColor='primary'
                textAlign='right'
              >
                {`${formattedAssetBalance} ${asset.symbol}`}
              </Text>
              <Text
                textSize='12px'
                isBold={false}
                textColor='secondary'
                textAlign='right'
              >
                {fiatBalance.formatAsFiat(defaultFiatCurrency)}
              </Text>
            </WithHideBalancePlaceholder>
          </Column>
        </AccountButton>
        <AccountMenuWrapper ref={accountMenuRef}>
          <AccountMenuButton
            onClick={() => setShowAccountMenu((prev) => !prev)}
          >
            <AccountMenuIcon />
          </AccountMenuButton>
          {showAccountMenu && (
            <>
              {isRewardsAccount ? (
                <RewardsMenu />
              ) : (
                <PortfolioAccountMenu
                  onClickViewOnExplorer={
                    blockExplorerSupported
                      ? onViewAccountOnBlockExplorer
                      : undefined
                  }
                  onClickSell={
                    isSellSupported && !isAssetsBalanceZero
                      ? showSellModal
                      : undefined
                  }
                  onClickDeposit={onShowDepositModal}
                />
              )}
            </>
          )}
        </AccountMenuWrapper>
      </StyledWrapper>

      {showDepositModal && (
        <PopupModal
          title={getLocale('braveWalletDepositCryptoButton')}
          onClose={() => setShowDepositModal(false)}
          ref={depositModalRef}
        >
          <VerticalDivider />
          <Column
            fullHeight={true}
            fullWidth={true}
            justifyContent='flex-start'
            padding='20px 15px'
          >
            <DepositModal selectedAccount={account} />
          </Column>
        </PopupModal>
      )}
    </>
  )
}

export default PortfolioAccountItem
