import * as React from 'react'
import { WalletAccountType } from '../connect-with-site-panel'

// Styled Components
import {
  StyledWrapper,
  AccountAddressText,
  AccountNameText,
  NameAndAddressColumn,
  AccountCircle,
  LeftSide,
  SelectedCircle
} from './style'

export interface Props {
  account: WalletAccountType
  isSelected: boolean
  action: () => void
}

export default class SelectAddress extends React.PureComponent<Props> {
  reduceAddress = (address: string) => {
    const firstHalf = address.slice(0, 6)
    const secondHalf = address.slice(-4)
    return firstHalf.concat('***', secondHalf)
  }

  render () {
    const { account, isSelected, action } = this.props
    return (
      <StyledWrapper onClick={action}>
        <LeftSide>
          <AccountCircle />
          <NameAndAddressColumn>
            <AccountNameText>{account.name}</AccountNameText>
            <AccountAddressText>
              {this.reduceAddress(account.address)}
            </AccountAddressText>
          </NameAndAddressColumn>
        </LeftSide>
        <SelectedCircle isSelected={isSelected} />
      </StyledWrapper>
    )
  }
}
