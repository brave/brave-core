import * as React from 'react'
import { getLocale } from '../../../../common/locale'
import PanelTooltip from '../panel-tooltip'
import { reduceNetworkDisplayName } from '../../../utils/network-utils'

// Styled Components
import {
  StyledWrapper,
  LightningBoltIcon,
  NavButton,
  NavButtonText,
  NavDivider,
  NavOutline,
  TransactionsButton
} from './style'

import { BraveWallet, PanelTypes } from '../../../constants/types'

export interface Props {
  onNavigate: (path: PanelTypes) => void
  isSwapDisabled: boolean
  isBuyDisabled: boolean
  selectedNetwork: BraveWallet.EthereumChain
}

function ConnectedBottomNav (props: Props) {
  const { onNavigate, isSwapDisabled, isBuyDisabled, selectedNetwork } = props

  const navigate = (path: PanelTypes) => () => {
    onNavigate(path)
  }

  return (
    <StyledWrapper>
      <NavOutline>
        <PanelTooltip
          position='right'
          isDisabled={isBuyDisabled}
          text={getLocale('braveWalletBssToolTip').replace('$1', reduceNetworkDisplayName(selectedNetwork.chainName))}
        >
          <NavButton disabled={isBuyDisabled} onClick={navigate('buy')}>
            <NavButtonText disabled={isBuyDisabled}>{getLocale('braveWalletBuy')}</NavButtonText>
          </NavButton>
        </PanelTooltip>
        <NavDivider />
        <NavButton onClick={navigate('send')}>
          <NavButtonText>{getLocale('braveWalletSend')}</NavButtonText>
        </NavButton>
        <NavDivider />
        <PanelTooltip
          position='left'
          isDisabled={isSwapDisabled}
          text={getLocale('braveWalletBssToolTip').replace('$1', reduceNetworkDisplayName(selectedNetwork.chainName))}
        >
          <NavButton disabled={isSwapDisabled} onClick={navigate('swap')}>
            <NavButtonText disabled={isSwapDisabled}>{getLocale('braveWalletSwap')}</NavButtonText>
          </NavButton>
        </PanelTooltip>
        <NavDivider />
        <TransactionsButton onClick={navigate('transactions')}>
          <LightningBoltIcon />
        </TransactionsButton>
      </NavOutline>
    </StyledWrapper>
  )
}

export default ConnectedBottomNav
