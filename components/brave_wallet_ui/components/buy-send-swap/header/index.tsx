import * as React from 'react'
import { useSelector } from 'react-redux'
import { BuySendSwapViewTypes, WalletState } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { create } from 'ethereum-blockies'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { Tooltip, SelectNetworkButton } from '../../shared'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  StyledWrapper,
  AccountAddress,
  AccountAndAddress,
  AccountCircle,
  AccountName,
  NameAndIcon,
  SwitchIcon
} from './style'

export interface Props {
  onChangeSwapView: (view: BuySendSwapViewTypes) => void
}

function SwapHeader (props: Props) {
  // redux
  const {
    selectedAccount,
    selectedNetwork
  } = useSelector((state: {wallet: WalletState}) => {
    return state.wallet
  })

  const { onChangeSwapView } = props

  const onShowAccounts = () => {
    onChangeSwapView('acounts')
  }

  const onShowNetworks = () => {
    onChangeSwapView('networks')
  }

  const onCopyToClipboard = async () => {
    await copyToClipboard(selectedAccount.address)
  }

  const orb = React.useMemo(() => {
    return create({ seed: selectedAccount.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [selectedAccount])

  return (
    <StyledWrapper>
      <NameAndIcon>
        <AccountCircle onClick={onShowAccounts} orb={orb}>
          <SwitchIcon />
        </AccountCircle>
        <Tooltip text={getLocale('braveWalletToolTipCopyToClipboard')}>
          <AccountAndAddress onClick={onCopyToClipboard}>
            <AccountName>{reduceAccountDisplayName(selectedAccount.name, 11)}</AccountName>
            <AccountAddress>{reduceAddress(selectedAccount.address)}</AccountAddress>
          </AccountAndAddress>
        </Tooltip>
      </NameAndIcon>
      <Tooltip text={selectedNetwork.chainName}>
        <SelectNetworkButton
          selectedNetwork={selectedNetwork}
          onClick={onShowNetworks}
        />
      </Tooltip>
    </StyledWrapper >
  )
}

export default SwapHeader
