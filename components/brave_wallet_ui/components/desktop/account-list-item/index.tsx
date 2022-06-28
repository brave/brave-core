import * as React from 'react'
import { reduceAddress } from '../../../utils/reduce-address'
import { create } from 'ethereum-blockies'
import {
  BraveWallet,
  WalletAccountType
} from '../../../constants/types'

import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'

// Styled Components
import {
  StyledWrapper,
  AccountName,
  AccountAddress,
  AccountAndAddress,
  NameAndIcon,
  AccountCircle,
  RightSide,
  HardwareIcon,
  AccountNameRow,
  DeleteButton,
  DeleteIcon
} from './style'

export interface Props {
  onDelete?: () => void
  onClick: (account: WalletAccountType) => void
  account: WalletAccountType
  isHardwareWallet: boolean
  onRemoveAccount: (address: string, hardware: boolean, coin: BraveWallet.CoinType) => void
}

function AccountListItem (props: Props) {
  const {
    account,
    isHardwareWallet,
    onClick,
    onRemoveAccount
  } = props

  const onSelectAccount = () => {
    onClick(account)
  }

  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])

  const removeAccount = () => {
    let confirmAction = confirm(`Are you sure to remove ${account.name}?`)
    if (confirmAction) {
      onRemoveAccount(account.address, isHardwareWallet, account.coin)
    }
  }

  return (
    <StyledWrapper>
      <NameAndIcon>
        <AccountCircle orb={orb} />
        <AccountAndAddress>
          <AccountNameRow>
            {isHardwareWallet && <HardwareIcon />}
            <AccountName onClick={onSelectAccount}>{account.name}</AccountName>
          </AccountNameRow>
          <CopyTooltip text={account.address}>
            <AccountAddress>{reduceAddress(account.address)}</AccountAddress>
          </CopyTooltip>
        </AccountAndAddress>
      </NameAndIcon>
      <RightSide>
        {(account.accountType !== 'Primary') &&
          <DeleteButton onClick={removeAccount}>
            <DeleteIcon />
          </DeleteButton>
        }
      </RightSide>
    </StyledWrapper>
  )
}

export default AccountListItem
