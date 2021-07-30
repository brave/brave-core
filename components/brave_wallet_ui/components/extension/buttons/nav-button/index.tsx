import * as React from 'react'

// Styled Components
import { StyledButton, ButtonText } from './style'

export interface Props {
  buttonType: 'primary' | 'secondary' | 'danger'
  text: string | undefined
  onSubmit: () => void
  disabled?: boolean
}

export default class NavButton extends React.PureComponent<Props, {}> {
  render () {
    const { onSubmit, text, buttonType, disabled } = this.props
    return (
      <StyledButton disabled={disabled} buttonType={buttonType} onClick={onSubmit}>
        <ButtonText buttonType={buttonType}>{text}</ButtonText>
      </StyledButton>
    )
  }
}
