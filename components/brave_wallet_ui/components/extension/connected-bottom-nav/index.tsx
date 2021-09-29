import * as React from 'react'
import locale from '../../../constants/locale'
import PanelTooltip from '../panel-tooltip'
import { reduceNetworkDisplayName } from '../../../utils/network-utils'

// Styled Components
import {
  StyledWrapper,
  // AppsIcon,
  NavButton,
  NavButtonText,
  NavDivider,
  NavOutline
} from './style'

import { PanelTypes, EthereumChain } from '../../../constants/types'

export interface Props {
  onNavigate: (path: PanelTypes) => void
  isSwapDisabled: boolean
  isBuyDisabled: boolean
  selectedNetwork: EthereumChain
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
          text={`${reduceNetworkDisplayName(selectedNetwork.chainName)} ${locale.bssToolTip}`}
        >
          <NavButton disabled={isBuyDisabled} onClick={navigate('buy')}>
            <NavButtonText disabled={isBuyDisabled}>{locale.buy}</NavButtonText>
          </NavButton>
        </PanelTooltip>
        <NavDivider />
        <NavButton onClick={navigate('send')}>
          <NavButtonText>{locale.send}</NavButtonText>
        </NavButton>
        <NavDivider />
        <PanelTooltip
          position='left'
          isDisabled={isSwapDisabled}
          text={`${reduceNetworkDisplayName(selectedNetwork.chainName)} ${locale.bssToolTip}`}
        >
          <NavButton disabled={isSwapDisabled} onClick={navigate('swap')}>
            <NavButtonText disabled={isSwapDisabled}>{locale.swap}</NavButtonText>
          </NavButton>
        </PanelTooltip>
        {/* <NavDivider /> */}
        {/*Temp commented out for MVP*/}
        {/* <NavButton onClick={navigate('apps')}>
          <AppsIcon />
        </NavButton> */}
      </NavOutline>
    </StyledWrapper>
  )
}

export default ConnectedBottomNav
