// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { create } from 'ethereum-blockies'
import { useHistory } from 'react-router'

// Types
import { BraveWallet, DefaultCurrencies, WalletRoutes, AssetPriceWithContractAndChainId } from '../../../constants/types'

// Hooks
import { useExplorer, usePricing } from '../../../common/hooks'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'

// Components
import { TransactionPopup, WithHideBalancePlaceholder } from '../'
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'
import { TransactionPopupItem } from '../transaction-popup'

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
  spotPrices: AssetPriceWithContractAndChainId[]
  address: string
  defaultCurrencies: DefaultCurrencies
  assetContractAddress: string
  assetChainId: string
  assetBalance: string
  assetTicker: string
  assetDecimals: number
  selectedNetwork?: BraveWallet.NetworkInfo
  name: string
  hideBalances?: boolean
  isNft?: boolean
  isSellSupported: boolean
  showSellModal: () => void
}

export const PortfolioAccountItem = (props: Props) => {
  const {
    assetContractAddress,
    assetChainId,
    assetBalance,
    address,
    assetTicker,
    assetDecimals,
    selectedNetwork,
    defaultCurrencies,
    hideBalances,
    name,
    spotPrices,
    isNft,
    isSellSupported,
    showSellModal
  } = props

  // Routing
  const history = useHistory()

  // Hooks
  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)
  const { computeFiatAmount } = usePricing(spotPrices)

  // State
  const [showAccountPopup, setShowAccountPopup] = React.useState<boolean>(false)

  // Memos
  const orb = React.useMemo(() => {
    return create({ seed: address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [address])

  const formattedAssetBalance: string = React.useMemo(() => {
    return new Amount(assetBalance)
      .divideByDecimals(assetDecimals)
      .format(6, true)
  }, [assetBalance, assetDecimals])

  const fiatBalance: Amount = React.useMemo(() => {
    return computeFiatAmount(assetBalance, assetTicker, assetDecimals, assetContractAddress, assetChainId)
  }, [computeFiatAmount, assetDecimals, assetBalance, assetTicker, assetContractAddress, assetChainId])

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
            <AssetBalanceText>{`${formattedAssetBalance} ${assetTicker}`}</AssetBalanceText>
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
