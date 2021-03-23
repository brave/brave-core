import * as React from 'react'
import { NavButton } from '../'

// Styled Components
import { StyledWrapper, ButtonRow, DisclaimerText } from './style'

export interface Props {
  primaryAction: () => void
  secondaryAction: () => void
  primaryText: string
  secondaryText: string
  disabled?: boolean
}

export default class ConnectBottomNav extends React.PureComponent<Props> {
  render () {
    const {
      primaryAction,
      secondaryAction,
      primaryText,
      secondaryText,
      disabled
    } = this.props
    return (
      <StyledWrapper>
        <DisclaimerText>Only connect with sites you trust.</DisclaimerText>
        <ButtonRow>
          <NavButton
            text={secondaryText}
            onSubmit={secondaryAction}
            buttonType='secondary'
          />
          <NavButton
            disabled={disabled}
            text={primaryText}
            onSubmit={primaryAction}
            buttonType='primary'

          />
        </ButtonRow>
      </StyledWrapper>
    )
  }
}
