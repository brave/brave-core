// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { create } from 'ethereum-blockies'
import { useHistory } from 'react-router'

// Types
import {
  BraveWallet,
  DefaultCurrencies,
  WalletRoutes
} from '../../../constants/types'

// Hooks
import { useExplorer } from '../../../common/hooks'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'
import { getPriceIdForToken } from '../../../utils/api-utils'
import { computeFiatAmount } from '../../../utils/pricing-utils'

// Components
import { TransactionPopup, WithHideBalancePlaceholder } from '../'
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'
import { TransactionPopupItem } from '../transaction-popup'

// Queries
import {
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
  AccountCircle,
  MoreButton,
  MoreIcon,
  RightSide,
  CopyIcon,
  AddressAndButtonRow
} from './style'
import { SellButtonRow, SellButton } from '../../shared/style'

interface Props {
  address: string
  defaultCurrencies: DefaultCurrencies
  asset: BraveWallet.BlockchainToken
  assetBalance: string
  selectedNetwork?: BraveWallet.NetworkInfo
  name: string
  hideBalances?: boolean
  isNft?: boolean
  isSellSupported: boolean
  showSellModal: () => void
}

export const PortfolioAccountItem = (props: Props) => {
  const {
    asset,
    assetBalance,
    address,
    selectedNetwork,
    defaultCurrencies,
    hideBalances,
    name,
    isNft,
    isSellSupported,
    showSellModal
  } = props

  // Routing
  const history = useHistory()

  // Hooks
  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  // State
  const [showAccountPopup, setShowAccountPopup] = React.useState<boolean>(false)

  // Memos
  const orb = React.useMemo(() => {
    return create({ seed: address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [address])

  const formattedAssetBalance: string = React.useMemo(() => {
    return new Amount(assetBalance)
      .divideByDecimals(asset.decimals)
      .format(6, true)
  }, [assetBalance, asset.decimals])

  const tokenPriceIds = React.useMemo(
    () => [getPriceIdForToken(asset)],
    [asset]
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    { ids: tokenPriceIds },
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
  const onHideAccountPopup = React.useCallback(() => {
    if (showAccountPopup) {
      setShowAccountPopup(false)
    }
  }, [showAccountPopup])

  const onSelectAccount = React.useCallback(() => {
    history.push(`${WalletRoutes.Accounts}/${address}`)
  }, [address])

  return (
    <StyledWrapper onClick={onHideAccountPopup}>
      <NameAndIcon>
        <AccountCircle orb={orb} />
        <AccountAndAddress>
          <AccountNameButton onClick={onSelectAccount}>{name}</AccountNameButton>
          <AddressAndButtonRow>
            <AccountAddressButton onClick={onSelectAccount}>{reduceAddress(address)}</AccountAddressButton>
            <CopyTooltip text={address}>
              <CopyIcon />
            </CopyTooltip>
          </AddressAndButtonRow>
        </AccountAndAddress>

      </NameAndIcon>
      <RightSide>
        <BalanceColumn>
          <WithHideBalancePlaceholder
            size='small'
            hideBalances={hideBalances ?? false}
          >
            {!isNft &&
              <FiatBalanceText>
                {fiatBalance.formatAsFiat(defaultCurrencies.fiat)}
              </FiatBalanceText>
            }
            <AssetBalanceText>
              {`${formattedAssetBalance} ${asset.symbol}`}
            </AssetBalanceText>
          </WithHideBalancePlaceholder>
        </BalanceColumn>
        <SellButtonRow>
          {isSellSupported && !isAssetsBalanceZero &&
            <SellButton onClick={showSellModal}>{getLocale('braveWalletSell')}</SellButton>
          }
        </SellButtonRow>
        <MoreButton onClick={() => setShowAccountPopup(true)}>
          <MoreIcon />
        </MoreButton>
        {showAccountPopup &&
          <TransactionPopup>
            <TransactionPopupItem
              onClick={onClickViewOnBlockExplorer('address', address)}
              text={getLocale('braveWalletTransactionExplorer')}
            />
          </TransactionPopup>
        }
      </RightSide>
    </StyledWrapper>
  )
}

export default PortfolioAccountItem
