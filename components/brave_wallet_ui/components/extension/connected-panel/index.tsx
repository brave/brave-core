import * as React from 'react'

// Components
import {
  ConnectedBottomNav,
  ConnectedHeader
} from '../'
import { Tooltip } from '../../shared'

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
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { WalletAccountType, PanelTypes, Network } from '../../../constants/types'
import { create, background } from 'ethereum-blockies'
import locale from '../../../constants/locale'
import { NetworkOptions } from '../../../options/network-options'

export interface Props {
  selectedAccount: WalletAccountType
  selectedNetwork: Network
  isConnected: boolean
  connectAction: () => void
  navAction: (path: PanelTypes) => void
}

const ConnectedPanel = (props: Props) => {
  const { connectAction, isConnected, navAction, selectedAccount, selectedNetwork } = props

  const navigate = (path: PanelTypes) => () => {
    navAction(path)
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

  const FiatBalance = selectedAccount.balance * 2000
  return (
    <StyledWrapper panelBackground={bg}>
      <ConnectedHeader action={navAction} />
      <CenterColumn>
        <StatusRow>
          <OvalButton onClick={connectAction}>
            {isConnected ? (<ConnectedIcon />) : (<NotConnectedIcon />)}
            <OvalButtonText>{isConnected ? 'Connected' : 'Not Connected'}</OvalButtonText>
          </OvalButton>
          <OvalButton onClick={navigate('networks')}>
            <OvalButtonText>{NetworkOptions[selectedNetwork].abbr}</OvalButtonText>
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
          <AssetBalanceText>{selectedAccount.balance} {selectedAccount.asset.toUpperCase()}</AssetBalanceText>
          <FiatBalanceText>${FiatBalance.toFixed(2)}</FiatBalanceText>
        </BalanceColumn>
      </CenterColumn>
      <ConnectedBottomNav action={navAction} />
    </StyledWrapper>
  )
}

export default ConnectedPanel
