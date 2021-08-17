import * as React from 'react'

// Components
import {
  ConnectedBottomNav,
  ConnectedHeader
} from '../'
import { Tooltip } from '../../shared'
import { formatPrices } from '../../../utils/format-prices'
import { formatBalance } from '../../../utils/format-balances'

// Styled Components
import {
  StyledWrapper,
  AssetBalanceText,
  FiatBalanceText,
  AccountCircle,
  AccountAddressText,
  AccountNameText,
  CenterColumn,
  SwapIcon,
  OvalButton,
  OvalButtonText,
  ConnectedIcon,
  NotConnectedIcon,
  CaratDownIcon,
  StatusRow,
  BalanceColumn
} from './style'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceNetworkDisplayName } from '../../../utils/network-utils'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { WalletAccountType, PanelTypes, EthereumChain } from '../../../constants/types'
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
    return background({ seed: selectedAccount.address })
  }, [selectedAccount.address])

  const orb = React.useMemo(() => {
    return create({ seed: selectedAccount.address, size: 8, scale: 16 }).toDataURL()
  }, [selectedAccount.address])

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
            {isConnected ? (<ConnectedIcon />) : (<NotConnectedIcon />)}
            <OvalButtonText>{isConnected ? locale.panelConnected : locale.panelNotConnected}</OvalButtonText>
          </OvalButton>
          <OvalButton onClick={navigate('networks')}>
            <OvalButtonText>{reduceNetworkDisplayName(selectedNetwork.chainName)}</OvalButtonText>
            <CaratDownIcon />
          </OvalButton>
        </StatusRow>
        <BalanceColumn>
          <AccountCircle orb={orb} />
          <AccountNameText>{selectedAccount.name}</AccountNameText>
          <Tooltip text={locale.toolTipCopyToClipboard}>
            <AccountAddressText onClick={onCopyToClipboard}>{reduceAddress(selectedAccount.address)}</AccountAddressText>
          </Tooltip>
        </BalanceColumn>
        <OvalButton onClick={navigate('accounts')}><SwapIcon /></OvalButton>
        <BalanceColumn>
          <AssetBalanceText>{formatBalance(selectedAccount.balance, 18)} {selectedAccount.asset.toUpperCase()}</AssetBalanceText>
          <FiatBalanceText>${formatPrices(Number(selectedAccount.fiatBalance))}</FiatBalanceText>
        </BalanceColumn>
      </CenterColumn>
      <ConnectedBottomNav action={navAction} />
    </StyledWrapper>
  )
}

export default ConnectedPanel
