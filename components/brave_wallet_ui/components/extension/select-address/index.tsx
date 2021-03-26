import * as React from 'react'
import { WalletAccountType } from '../../../constants/types'

// Styled Components
import {
  StyledWrapper,
  AccountAddressText,
  AccountNameText,
  NameAndAddressColumn,
  AccountCircle,
  LeftSide,
  UnSelectedCircle,
  SelectedIcon
} from './style'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'

export interface Props {
  account: WalletAccountType
  isSelected: boolean
  action: () => void
}

export default class SelectAddress extends React.PureComponent<Props> {
  render () {
    const { account, isSelected, action } = this.props
    return (
      <StyledWrapper onClick={action}>
        <LeftSide>
          <AccountCircle />
          <NameAndAddressColumn>
            <AccountNameText>{account.name}</AccountNameText>
            <AccountAddressText>
              {reduceAddress(account.address)}
            </AccountAddressText>
          </NameAndAddressColumn>
        </LeftSide>
        {isSelected ? <SelectedIcon /> : <UnSelectedCircle />}
      </StyledWrapper>
    )
  }
}
