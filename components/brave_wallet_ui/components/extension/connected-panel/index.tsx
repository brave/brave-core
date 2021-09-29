import * as React from 'react'

// Components
import {
  ConnectedBottomNav,
  ConnectedHeader
} from '../'
import { Tooltip } from '../../shared'
import { formatPrices } from '../../../utils/format-prices'
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
  SwitchIcon
} from './style'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceNetworkDisplayName } from '../../../utils/network-utils'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { WalletAccountType, PanelTypes, EthereumChain, BuySupportedChains, SwapSupportedChains } from '../../../constants/types'
import { create, background } from 'ethereum-blockies'
import locale from '../../../constants/locale'

export interface Props {
  selectedAccount: WalletAccountType
  selectedNetwork: EthereumChain
  isConnected: boolean
  connectAction: () => void
  navAction: (path: PanelTypes) => void
  onLockWallet: () => void
  onOpenSettings: () => void
}

const ConnectedPanel = (props: Props) => {
  const { onLockWallet, onOpenSettings, connectAction, isConnected, navAction, selectedAccount, selectedNetwork } = props
  const [showMore, setShowMore] = React.useState<boolean>(false)

  const navigate = (path: PanelTypes) => () => {
    navAction(path)
  }

  const onExpand = () => {
    navAction('expanded')
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

  return (
    <StyledWrapper onClick={onHideMore} panelBackground={bg}>
      <ConnectedHeader
        onExpand={onExpand}
        onClickLock={onLockWallet}
        onClickSetting={onOpenSettings}
        onClickMore={onShowMore}
        showMore={showMore}
      />
      <CenterColumn>
        <StatusRow>
          <OvalButton onClick={connectAction}>
            {isConnected && <BigCheckMark />}
            <OvalButtonText>{isConnected ? locale.panelConnected : locale.panelNotConnected}</OvalButtonText>
          </OvalButton>
          <OvalButton onClick={navigate('networks')}>
            <OvalButtonText>{reduceNetworkDisplayName(selectedNetwork.chainName)}</OvalButtonText>
            <CaratDownIcon />
          </OvalButton>
        </StatusRow>
        <BalanceColumn>
          <AccountCircle orb={orb} onClick={navigate('accounts')}>
            <SwitchIcon />
          </AccountCircle>
          <AccountNameText>{reduceAccountDisplayName(selectedAccount.name, 14)}</AccountNameText>
          <Tooltip text={locale.toolTipCopyToClipboard}>
            <AccountAddressText onClick={onCopyToClipboard}>{reduceAddress(selectedAccount.address)}</AccountAddressText>
          </Tooltip>
        </BalanceColumn>
        <BalanceColumn>
          <AssetBalanceText>{formatBalance(selectedAccount.balance, 18)} {selectedAccount.asset.toUpperCase()}</AssetBalanceText>
          <FiatBalanceText>${formatPrices(Number(selectedAccount.fiatBalance))}</FiatBalanceText>
        </BalanceColumn>
      </CenterColumn>
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
