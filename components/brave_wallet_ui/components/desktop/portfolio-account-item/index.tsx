import * as React from 'react'
import { create } from 'ethereum-blockies'

// Hooks
import { useExplorer, usePricing } from '../../../common/hooks'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import Amount from '../../../utils/amount'

import { getLocale } from '../../../../common/locale'
import { BraveWallet, DefaultCurrencies } from '../../../constants/types'
import { TransactionPopup, WithHideBalancePlaceholder } from '../'
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'

// Styled Components
import {
  StyledWrapper,
  AssetBalanceText,
  AccountName,
  AccountAddress,
  AccountAndAddress,
  BalanceColumn,
  FiatBalanceText,
  NameAndIcon,
  AccountCircle,
  MoreButton,
  MoreIcon,
  RightSide
} from './style'
import { TransactionPopupItem } from '../transaction-popup'

export interface Props {
  spotPrices: BraveWallet.AssetPrice[]
  defaultCurrencies: DefaultCurrencies
  address: string
  assetBalance: string
  assetTicker: string
  assetDecimals: number
  selectedNetwork: BraveWallet.NetworkInfo
  name: string
  hideBalances?: boolean
}

const PortfolioAccountItem = (props: Props) => {
  const {
    address,
    name,
    assetBalance,
    assetTicker,
    assetDecimals,
    selectedNetwork,
    defaultCurrencies,
    hideBalances,
    spotPrices
  } = props
  const [showAccountPopup, setShowAccountPopup] = React.useState<boolean>(false)

  const orb = React.useMemo(() => {
    return create({ seed: address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [address])

  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  const formattedAssetBalance = new Amount(assetBalance)
    .divideByDecimals(assetDecimals)
    .format(6, true)

  const { computeFiatAmount } = usePricing(spotPrices)
  const fiatBalance = React.useMemo(() => {
    return computeFiatAmount(assetBalance, assetTicker, assetDecimals)
  }, [computeFiatAmount, assetDecimals, assetBalance, assetTicker])

  const onShowTransactionPopup = () => {
    setShowAccountPopup(true)
  }

  const onHideTransactionPopup = () => {
    if (showAccountPopup) {
      setShowAccountPopup(false)
    }
  }

  return (
    <StyledWrapper onClick={onHideTransactionPopup}>
      <NameAndIcon>
        <AccountCircle orb={orb} />
        <CopyTooltip text={address}>
          <AccountAndAddress>
            <AccountName>{name}</AccountName>
            <AccountAddress>{reduceAddress(address)}</AccountAddress>
          </AccountAndAddress>
        </CopyTooltip>
      </NameAndIcon>
      <RightSide>
        <BalanceColumn>
          <WithHideBalancePlaceholder
            size='small'
            hideBalances={hideBalances ?? false}
          >
            <FiatBalanceText>
              {fiatBalance.formatAsFiat(defaultCurrencies.fiat)}
            </FiatBalanceText>
            <AssetBalanceText>{`${formattedAssetBalance} ${assetTicker}`}</AssetBalanceText>
          </WithHideBalancePlaceholder>
        </BalanceColumn>
        <MoreButton onClick={onShowTransactionPopup}>
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
