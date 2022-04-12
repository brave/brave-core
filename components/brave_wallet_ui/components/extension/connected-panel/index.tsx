import * as React from 'react'

// Components
import {
  ConnectedBottomNav,
  ConnectedHeader
} from '../'
import { Tooltip, SelectNetworkButton } from '../../shared'

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
  StatusRow,
  BalanceColumn,
  SwitchIcon,
  MoreAssetsButton
} from './style'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import Amount from '../../../utils/amount'

// Hooks
import { useExplorer, usePricing } from '../../../common/hooks'

import {
  WalletAccountType,
  PanelTypes,
  BraveWallet,
  BuySupportedChains,
  DefaultCurrencies
} from '../../../constants/types'
import { create, background } from 'ethereum-blockies'
import { getLocale } from '../../../../common/locale'

export interface Props {
  selectedAccount: WalletAccountType
  selectedNetwork: BraveWallet.NetworkInfo
  isConnected: boolean
  originInfo: BraveWallet.OriginInfo
  isSwapSupported: boolean
  defaultCurrencies: DefaultCurrencies
  navAction: (path: PanelTypes) => void
  onLockWallet: () => void
  onOpenSettings: () => void
}

const ConnectedPanel = (props: Props) => {
  const {
    onLockWallet,
    onOpenSettings,
    isConnected,
    isSwapSupported,
    navAction,
    selectedAccount,
    selectedNetwork,
    originInfo,
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

  const formattedAssetBalance = new Amount(selectedAccount.nativeBalanceRegistry[selectedNetwork.chainId] ?? '')
    .divideByDecimals(selectedNetwork.decimals)
    .formatAsAsset(6, selectedNetwork.symbol)

  const { computeFiatAmount } = usePricing()

  const selectedAccountFiatBalance = React.useMemo(() => computeFiatAmount(
    selectedAccount.nativeBalanceRegistry[selectedNetwork.chainId], selectedNetwork.symbol, selectedNetwork.decimals
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
          {originInfo?.origin?.scheme !== 'chrome' ? (
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
            <SelectNetworkButton
              onClick={navigate('networks')}
              selectedNetwork={selectedNetwork}
              isPanel={true}
            />
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
          <AssetBalanceText>{formattedAssetBalance}</AssetBalanceText>
          <FiatBalanceText>
            {selectedAccountFiatBalance.formatAsFiat(defaultCurrencies.fiat)}
          </FiatBalanceText>
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
