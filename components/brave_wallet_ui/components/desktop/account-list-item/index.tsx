import * as React from 'react'
import { reduceAddress } from '../../../utils/reduce-address'
import { create } from 'ethereum-blockies'
import { Tooltip } from '../../shared'
import {
  BraveWallet,
  WalletAccountType
} from '../../../constants/types'
import { getLocale } from '../../../../common/locale'
import { useCopy } from '../../../common/hooks'

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

  // custom hooks
  const { copied, copyText } = useCopy()

  const onCopyToClipboard = async () => {
    await copyText(account.address)
  }

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
          <Tooltip
            text={getLocale('braveWalletToolTipCopyToClipboard')}
            actionText={getLocale('braveWalletToolTipCopiedToClipboard')}
            isActionVisible={copied}
          >
            <AccountAddress onClick={onCopyToClipboard}>{reduceAddress(account.address)}</AccountAddress>
          </Tooltip>
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
