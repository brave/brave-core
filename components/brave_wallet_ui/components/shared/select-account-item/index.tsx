import * as React from 'react'
import { create } from 'ethereum-blockies'

// types
import { UserAccountType } from '../../../constants/types'

// utils
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'

// components
import Tooltip from '../tooltip'

// style
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
  selectedAccount?: UserAccountType
  onSelectAccount?: () => void
  showTooltips?: boolean
  fullAddress?: boolean
}

export function SelectAccountItem (props: Props) {
  const {
    account,
    selectedAccount,
    onSelectAccount,
    showTooltips,
    fullAddress
  } = props

  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account])

  const PossibleToolTip = React.useMemo(() => {
    return showTooltips ? Tooltip : ({ children, text, isAddress }: React.PropsWithChildren<{
      text: string
      isAddress?: boolean
    }>) => (children || <></>) as JSX.Element
  }, [showTooltips])

  return (
    <StyledWrapper onClick={onSelectAccount}>
      <LeftSide>
        <AccountCircle orb={orb} />
        <AccountAndAddress>
          <PossibleToolTip text={account.name} isAddress>
            <AccountName>{reduceAccountDisplayName(account.name, 22)}</AccountName>
          </PossibleToolTip>
          <PossibleToolTip text={account.address} isAddress>
            <AccountAddress>{fullAddress ? account.address : reduceAddress(account.address)}</AccountAddress>
          </PossibleToolTip>
        </AccountAndAddress>
      </LeftSide>
      {account.address.toLowerCase() === selectedAccount?.address.toLowerCase() &&
        <BigCheckMark />
      }
    </StyledWrapper>
  )
}

export default SelectAccountItem
