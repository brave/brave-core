import * as React from 'react'
import { UserAccountType } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { create } from 'ethereum-blockies'
// Styled Components
import {
  StyledWrapper,
  AccountAddress,
  AccountAndAddress,
  AccountCircle,
  AccountName,
  LeftSide,
  BigCheckMark
} from './style'

export interface Props {
  account: UserAccountType
  selectedAccount: UserAccountType
  onSelectAccount: () => void
}

function SelectAccountItem (props: Props) {
  const { account, selectedAccount, onSelectAccount } = props

  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account])

  return (
    <StyledWrapper onClick={onSelectAccount}>
      <LeftSide>
        <AccountCircle orb={orb} />
        <AccountAndAddress>
          <AccountName>{reduceAccountDisplayName(account.name, 22)}</AccountName>
          <AccountAddress>{reduceAddress(account.address)}</AccountAddress>
        </AccountAndAddress>
      </LeftSide>
      {account.address.toLowerCase() === selectedAccount.address.toLowerCase() &&
        <BigCheckMark />
      }
    </StyledWrapper>
  )
}

export default SelectAccountItem
