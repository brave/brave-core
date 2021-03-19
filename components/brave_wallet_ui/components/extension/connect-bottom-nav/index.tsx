import * as React from 'react'
import { NavButton } from '../'

// Styled Components
import { StyledWrapper, ButtonRow, DisclaimerText } from './style'

export interface Props {
  onSubmit: () => void
  onCancel: () => void
  actionText: string
}

export default class ConnectBottomNav extends React.PureComponent<Props> {
  render () {
    const { onSubmit, onCancel, actionText } = this.props
    return (
      <StyledWrapper>
        <DisclaimerText>Only connect with sites you trust.</DisclaimerText>
        <ButtonRow>
          <NavButton text='Cancel' onSubmit={onCancel} buttonType='secondary' />
          <NavButton text={actionText} onSubmit={onSubmit} buttonType='primary' />
        </ButtonRow>
      </StyledWrapper>
    )
  }
}
