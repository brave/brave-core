import * as React from 'react'
import locale from '../../../constants/locale'

// Styled Components
import {
  StyledWrapper,
  AppsIcon,
  NavButton,
  NavButtonText,
  NavDivider,
  NavOutline
} from './style'

import { PanelTypes } from '../../../constants/types'

export interface Props {
  onNavigate: (path: PanelTypes) => void
}

function ConnectedBottomNav (props: Props) {
  const { onNavigate } = props

  const navigate = (path: PanelTypes) => () => {
    onNavigate(path)
  }

  return (
    <StyledWrapper>
      <NavOutline>
        <NavButton onClick={navigate('buy')}>
          <NavButtonText>{locale.buy}</NavButtonText>
        </NavButton>
        <NavDivider />
        <NavButton onClick={navigate('send')}>
          <NavButtonText>{locale.send}</NavButtonText>
        </NavButton>
        <NavDivider />
        <NavButton onClick={navigate('swap')}>
          <NavButtonText>{locale.swap}</NavButtonText>
        </NavButton>
        <NavDivider />
        <NavButton onClick={navigate('apps')}>
          <AppsIcon />
        </NavButton>
      </NavOutline>
    </StyledWrapper>
  )
}

export default ConnectedBottomNav
