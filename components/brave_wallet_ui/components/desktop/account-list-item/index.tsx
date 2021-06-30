import * as React from 'react'
import { reduceAddress } from '../../../utils/reduce-address'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { create } from 'ethereum-blockies'
import { Tooltip } from '../../shared'
import locale from '../../../constants/locale'

// Styled Components
import {
  StyledWrapper,
  AccountName,
  AccountAddress,
  AccountAndAddress,
  NameAndIcon,
  AccountCircle,
  DeleteButton,
  DeleteIcon,
  RightSide,
  HardwareIcon,
  AccountNameRow
} from './style'

export interface Props {
  onDelete: () => void
  onClick: () => void
  isHardwareWallet: boolean
  address: string
  name: string
}

function AccountListItem (props: Props) {
  const { address, name, isHardwareWallet, onDelete, onClick } = props

  const onCopyToClipboard = async () => {
    await copyToClipboard(address)
  }

  const orb = React.useMemo(() => {
    return create({ seed: address, size: 8, scale: 16 }).toDataURL()
  }, [address])

  return (
    <StyledWrapper>
      <NameAndIcon>
        <AccountCircle orb={orb} />
        <AccountAndAddress>
          <AccountNameRow>
            {isHardwareWallet && <HardwareIcon />}
            <AccountName onClick={onClick}>{name}</AccountName>
          </AccountNameRow>
          <Tooltip text={locale.toolTipCopyToClipboard}>
            <AccountAddress onClick={onCopyToClipboard}>{reduceAddress(address)}</AccountAddress>
          </Tooltip>
        </AccountAndAddress>
      </NameAndIcon>
      <RightSide>
        <DeleteButton onClick={onDelete}>
          <DeleteIcon />
        </DeleteButton>
      </RightSide>
    </StyledWrapper>
  )
}

export default AccountListItem
