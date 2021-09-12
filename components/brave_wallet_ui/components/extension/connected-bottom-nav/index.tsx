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
  isDisabled: boolean
  selectedNetwork: EthereumChain
}

function ConnectedBottomNav (props: Props) {
  const { onNavigate, isDisabled, selectedNetwork } = props

  const navigate = (path: PanelTypes) => () => {
    onNavigate(path)
  }

  return (
    <StyledWrapper>
      <NavOutline>
        <PanelTooltip
          position='right'
          isDisabled={isDisabled}
          text={`${reduceNetworkDisplayName(selectedNetwork.chainName)} ${locale.bssToolTip}`}
        >
          <NavButton disabled={isDisabled} onClick={navigate('buy')}>
            <NavButtonText disabled={isDisabled}>{locale.buy}</NavButtonText>
          </NavButton>
        </PanelTooltip>
        <NavDivider />
        <NavButton onClick={navigate('send')}>
          <NavButtonText>{locale.send}</NavButtonText>
        </NavButton>
        <NavDivider />
        <PanelTooltip
          position='left'
          isDisabled={isDisabled}
          text={`${reduceNetworkDisplayName(selectedNetwork.chainName)} ${locale.bssToolTip}`}
        >
          <NavButton disabled={isDisabled} onClick={navigate('swap')}>
            <NavButtonText disabled={isDisabled}>{locale.swap}</NavButtonText>
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
