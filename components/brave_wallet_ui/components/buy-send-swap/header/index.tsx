import * as React from 'react'
import { UserAccountType, BuySendSwapViewTypes, Network } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'
import { create } from 'ethereum-blockies'
import { NetworkOptions } from '../../../options/network-options'
// Styled Components
import {
  StyledWrapper,
  AccountAddress,
  AccountAndAddress,
  AccountCircle,
  AccountName,
  NameAndIcon,
  OvalButton,
  OvalButtonText,
  CaratDownIcon
} from './style'

export interface Props {
  selectedAccount: UserAccountType
  selectedNetwork: Network
  onChangeSwapView: (view: BuySendSwapViewTypes) => void
}

function SwapHeader (props: Props) {
  const { selectedAccount, selectedNetwork, onChangeSwapView } = props

  const onShowAccounts = () => {
    onChangeSwapView('acounts')
  }

  const onShowNetworks = () => {
    onChangeSwapView('networks')
  }

  const orb = React.useMemo(() => {
    return create({ seed: selectedAccount.address, size: 8, scale: 16 }).toDataURL()
  }, [selectedAccount])

  return (
    <StyledWrapper>
      <NameAndIcon>
        <AccountCircle orb={orb} />
        <AccountAndAddress onClick={onShowAccounts}>
          <AccountName>{selectedAccount.name}</AccountName>
          <AccountAddress>{reduceAddress(selectedAccount.address)}</AccountAddress>
        </AccountAndAddress>
      </NameAndIcon>

      <OvalButton onClick={onShowNetworks}>
        <OvalButtonText>{NetworkOptions[selectedNetwork].abbr}</OvalButtonText>
        <CaratDownIcon />
      </OvalButton>
    </StyledWrapper >
  )
}

export default SwapHeader
