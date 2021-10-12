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
  DisconnectButton
} from './style'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'

export interface Props {
  account: WalletAccountType
  onDisconnect: (address: string) => void
}

const ConnectedAccountItem = (props: Props) => {
  const { account, onDisconnect } = props

  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])

  const onClickDisconnect = () => {
    onDisconnect(account.address)
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
      <DisconnectButton onClick={onClickDisconnect}>{getLocale('braveWalletSitePermissionsDisconnect')}</DisconnectButton>
    </StyledWrapper>
  )
}

export default ConnectedAccountItem
