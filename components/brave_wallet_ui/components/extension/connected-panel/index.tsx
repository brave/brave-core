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
import { PortfolioAssetItem } from '../../desktop'

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
  ScrollContainer,
  MoreAssetsText,
  AssetContainer
} from './style'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceNetworkDisplayName } from '../../../utils/network-utils'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'

// Hooks
import { useExplorer } from '../../../common/hooks'

import {
  WalletAccountType,
  PanelTypes,
  BraveWallet,
  BuySupportedChains,
  SwapSupportedChains,
  WalletOrigin,
  DefaultCurrencies,
  WalletRoutes,
  AccountAssetOptionType
} from '../../../constants/types'
import { create, background } from 'ethereum-blockies'
import { getLocale } from '../../../../common/locale'

export interface Props {
  selectedAccount: WalletAccountType
  selectedNetwork: BraveWallet.EthereumChain
  isConnected: boolean
  activeOrigin: string
  defaultCurrencies: DefaultCurrencies
  userAssetList: AccountAssetOptionType[]
  navAction: (path: PanelTypes) => void
  onLockWallet: () => void
  onOpenSettings: () => void
}

const ConnectedPanel = (props: Props) => {
  const {
    onLockWallet,
    onOpenSettings,
    isConnected,
    navAction,
    userAssetList,
    selectedAccount,
    selectedNetwork,
    activeOrigin,
    defaultCurrencies
  } = props
  const [showMore, setShowMore] = React.useState<boolean>(false)
  const [isScrolled, setIsScrolled] = React.useState<boolean>(false)

  let scrollRef = React.useRef<HTMLDivElement | null>(null)

  const onScroll = () => {
    const scrollPosition = scrollRef.current
    if (scrollPosition !== null) {
      const { scrollTop } = scrollPosition
      if (scrollTop > 20) {
        setIsScrolled(true)
      } else {
        setIsScrolled(false)
      }
    }
  }

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

  const isSwapDisabled = React.useMemo(() => {
    return !SwapSupportedChains.includes(selectedNetwork.chainId)
  }, [SwapSupportedChains, selectedNetwork])

  const formatedAssetBalance = formatBalance(selectedAccount.balance, selectedNetwork.decimals)

  const formatedAssetBalanceWithDecimals = selectedAccount.balance
    ? formatTokenAmountWithCommasAndDecimals(formatedAssetBalance, selectedNetwork.symbol)
    : ''

  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  const onClickAsset = (symbol: string) => () => {
    const url = `brave://wallet${WalletRoutes.Portfolio}/${symbol}`
    chrome.tabs.create({ url: url }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

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
      <ScrollContainer ref={scrollRef} onScroll={onScroll}>
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
            <AssetBalanceText>{formatedAssetBalanceWithDecimals}</AssetBalanceText>
            <FiatBalanceText>{formatFiatAmountWithCommasAndDecimals(selectedAccount.fiatBalance, defaultCurrencies.fiat)}</FiatBalanceText>
          </BalanceColumn>
        </CenterColumn>
        <AssetContainer isScrolled={isScrolled}>
          {userAssetList?.map((asset) =>
            <PortfolioAssetItem
              defaultCurrencies={defaultCurrencies}
              action={onClickAsset(asset.asset.symbol)}
              key={asset.asset.contractAddress}
              assetBalance={asset.assetBalance}
              fiatBalance={asset.fiatBalance}
              token={asset.asset}
              isPanel={true}
            />
          )}
        </AssetContainer>
      </ScrollContainer>
      {userAssetList.length !== 0 &&
        <MoreAssetsText isScrolled={isScrolled}>{getLocale('braveWalletPanelScrollForMoreAssets')}</MoreAssetsText>
      }
      <ConnectedBottomNav
        selectedNetwork={selectedNetwork}
        isBuyDisabled={isBuyDisabled}
        isSwapDisabled={isSwapDisabled}
        onNavigate={navAction}
      />
    </StyledWrapper>
  )
}

export default ConnectedPanel
