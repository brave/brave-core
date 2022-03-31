import * as React from 'react'
import { WalletAccountType } from '../../../constants/types'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { create } from 'ethereum-blockies'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  StyledWrapper,
  AccountAddressText,
  AccountNameText,
  NameAndAddressColumn,
  AccountCircle,
  LeftSide,
  PrimaryButton,
  RightSide
} from './style'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'

export interface Props {
  isActive: boolean
  hasPermission: boolean
  account: WalletAccountType
  onDisconnect: (address: string) => void
  onConnect: (account: WalletAccountType) => void
  onSwitchAccount: (account: WalletAccountType) => void
}

const SitePermissionAccountItem = (props: Props) => {
  const {
    account,
    isActive,
    hasPermission,
    onDisconnect,
    onConnect,
    onSwitchAccount
  } = props

  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])

  const onClickDisconnect = () => {
    onDisconnect(account.address)
  }

  const onClickConnect = () => {
    onConnect(account)
  }

  const onClickSwitchAccount = () => {
    onSwitchAccount(account)
  }

  return (
    <StyledWrapper>
      <LeftSide>
        <AccountCircle orb={orb} />
        <NameAndAddressColumn>
          <AccountNameText>{reduceAccountDisplayName(account.name, 22)}</AccountNameText>
          <AccountAddressText>
            {reduceAddress(account.address)}
          </AccountAddressText>
        </NameAndAddressColumn>
      </LeftSide>
      <RightSide>
        <PrimaryButton
          onClick={
            hasPermission
              ? isActive
                ? onClickDisconnect
                : onClickSwitchAccount
              : onClickConnect
          }
        >
          {
            hasPermission
              ? isActive
                ? getLocale('braveWalletSitePermissionsDisconnect')
                : getLocale('braveWalletSitePermissionsSwitch')
              : getLocale('braveWalletAddAccountConnect')
          }
        </PrimaryButton>
      </RightSide>
    </StyledWrapper>
  )
}

export default SitePermissionAccountItem
