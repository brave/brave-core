import * as React from 'react'

// Components
import {
  ConnectedBottomNav,
  ConnectedHeader
} from '../'
import { Tooltip } from '../../shared'
import {
  formatFiatAmountWithCommasAndDecimals,
  formatTokenAmountWithCommasAndDecimals
} from '../../../utils/format-prices'
import { formatBalance } from '../../../utils/format-balances'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'

// Styled Components
import {
  StyledWrapper,
  AssetBalanceText,
  FiatBalanceText,
  AccountCircle,
  AccountAddressText,
  AccountNameText,
  CenterColumn,
  OvalButton,
  OvalButtonText,
  BigCheckMark,
  CaratDownIcon,
  StatusRow,
  BalanceColumn,
  SwitchIcon,
  MoreAssetsButton
} from './style'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceNetworkDisplayName } from '../../../utils/network-utils'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'

// Hooks
import { useExplorer, usePricing } from '../../../common/hooks'

import {
  WalletAccountType,
  PanelTypes,
  BraveWallet,
  BuySupportedChains,
  WalletOrigin,
  DefaultCurrencies
} from '../../../constants/types'
import { create, background } from 'ethereum-blockies'
import { getLocale } from '../../../../common/locale'

export interface Props {
  spotPrices: BraveWallet.AssetPrice[]
  selectedAccount: WalletAccountType
  selectedNetwork: BraveWallet.EthereumChain
  isConnected: boolean
  activeOrigin: string
  isSwapSupported: boolean
  defaultCurrencies: DefaultCurrencies
  navAction: (path: PanelTypes) => void
  onLockWallet: () => void
  onOpenSettings: () => void
}

const ConnectedPanel = (props: Props) => {
  const {
    spotPrices,
    onLockWallet,
    onOpenSettings,
    isConnected,
    isSwapSupported,
    navAction,
    selectedAccount,
    selectedNetwork,
    activeOrigin,
    defaultCurrencies
  } = props
  const [showMore, setShowMore] = React.useState<boolean>(false)

  const navigate = (path: PanelTypes) => () => {
    navAction(path)
  }

  const onExpand = () => {
    navAction('expanded')
  }

  const onShowSitePermissions = () => {
    navAction('sitePermissions')
  }

  const onShowMore = () => {
    setShowMore(true)
  }

  const onHideMore = () => {
    if (showMore) {
      setShowMore(false)
    }
  }

  const onCopyToClipboard = async () => {
    await copyToClipboard(selectedAccount.address)
  }

  const bg = React.useMemo(() => {
    return background({ seed: selectedAccount.address.toLowerCase() })
  }, [selectedAccount.address])

  const orb = React.useMemo(() => {
    return create({ seed: selectedAccount.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [selectedAccount.address])

  const isBuyDisabled = React.useMemo(() => {
    return !BuySupportedChains.includes(selectedNetwork.chainId)
  }, [BuySupportedChains, selectedNetwork])

  const formattedAssetBalance = formatBalance(selectedAccount.balance, selectedNetwork.decimals)

  const formattedAssetBalanceWithDecimals = selectedAccount.balance
    ? formatTokenAmountWithCommasAndDecimals(formattedAssetBalance, selectedNetwork.symbol)
    : ''

  const { computeFiatAmount } = usePricing(spotPrices)

  const selectedAccountFiatBalance = React.useMemo(() => computeFiatAmount(
    selectedAccount.balance, selectedNetwork.symbol, selectedNetwork.decimals
  ), [computeFiatAmount, selectedNetwork, selectedAccount])

  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  return (
    <StyledWrapper onClick={onHideMore} panelBackground={bg}>
      <ConnectedHeader
        onExpand={onExpand}
        onClickLock={onLockWallet}
        onClickSetting={onOpenSettings}
        onClickMore={onShowMore}
        onClickViewOnBlockExplorer={onClickViewOnBlockExplorer('address', selectedAccount.address)}
        showMore={showMore}
      />
      <CenterColumn>
        <StatusRow>
          {activeOrigin !== WalletOrigin ? (
            <OvalButton onClick={onShowSitePermissions}>
              {isConnected && <BigCheckMark />}
              <OvalButtonText>{isConnected ? getLocale('braveWalletPanelConnected') : getLocale('braveWalletPanelNotConnected')}</OvalButtonText>
            </OvalButton>
          ) : (
            <div />
          )}
          <Tooltip
            text={selectedNetwork.chainName}
            positionRight={true}
          >
            <OvalButton onClick={navigate('networks')}>
              <OvalButtonText>{reduceNetworkDisplayName(selectedNetwork.chainName)}</OvalButtonText>
              <CaratDownIcon />
            </OvalButton>
          </Tooltip>
        </StatusRow>
        <BalanceColumn>
          <AccountCircle orb={orb} onClick={navigate('accounts')}>
            <SwitchIcon />
          </AccountCircle>
          <AccountNameText>{reduceAccountDisplayName(selectedAccount.name, 14)}</AccountNameText>
          <Tooltip text={getLocale('braveWalletToolTipCopyToClipboard')}>
            <AccountAddressText onClick={onCopyToClipboard}>{reduceAddress(selectedAccount.address)}</AccountAddressText>
          </Tooltip>
        </BalanceColumn>
        <BalanceColumn>
          <AssetBalanceText>{formattedAssetBalanceWithDecimals}</AssetBalanceText>
          <FiatBalanceText>{formatFiatAmountWithCommasAndDecimals(selectedAccountFiatBalance, defaultCurrencies.fiat)}</FiatBalanceText>
        </BalanceColumn>
        <MoreAssetsButton onClick={navigate('assets')}>{getLocale('braveWalletPanelViewAccountAssets')}</MoreAssetsButton>
      </CenterColumn>
      <ConnectedBottomNav
        selectedNetwork={selectedNetwork}
        isBuyDisabled={isBuyDisabled}
        isSwapDisabled={!isSwapSupported}
        onNavigate={navAction}
      />
    </StyledWrapper>
  )
}

export default ConnectedPanel
