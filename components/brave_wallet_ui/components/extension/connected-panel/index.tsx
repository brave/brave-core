import * as React from 'react'

// Components
import {
  ConnectedBottomNav,
  ConnectedHeader
} from '../'

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
import { WalletAccountType } from '../../../constants/types'

export interface Props {
  selectedAccount: WalletAccountType
  isConnected: boolean
  connectAction: () => void
  navAction: (path: string) => void
}

export default class ConnectedPanel extends React.PureComponent<Props> {

  navigate = (path: string) => () => {
    this.props.navAction(path)
  }

  render () {
    const FiatBalance = this.props.selectedAccount.balance * 2000
    const { selectedAccount, isConnected, connectAction, navAction } = this.props
    return (
      <StyledWrapper>
        <ConnectedHeader action={navAction} />
        <CenterColumn>
          <StatusRow>
            <OvalButton onClick={connectAction}>
              {isConnected ? (<ConnectedIcon />) : (<NotConnectedIcon />)}
              <OvalButtonText>{isConnected ? 'Connected' : 'Not Connected'}</OvalButtonText>
            </OvalButton>
            <OvalButton onClick={this.navigate('networks')}>
              <OvalButtonText>Mainnet</OvalButtonText>
              <CaratDownIcon />
            </OvalButton>
          </StatusRow>
          <BalanceColumn>
            <AccountCircle />
            <AccountNameText>{selectedAccount.name}</AccountNameText>
            <AccountAddressText>{reduceAddress(selectedAccount.address)}</AccountAddressText>
          </BalanceColumn>
          <OvalButton onClick={this.navigate('swap')}><SwapIcon /></OvalButton>
          <BalanceColumn>
            <AssetBalanceText>{selectedAccount.balance} {selectedAccount.asset.toUpperCase()}</AssetBalanceText>
            <FiatBalanceText>${FiatBalance.toFixed(2)}</FiatBalanceText>
          </BalanceColumn>
        </CenterColumn>
        <ConnectedBottomNav action={navAction} />
      </StyledWrapper>
    )
  }
}
