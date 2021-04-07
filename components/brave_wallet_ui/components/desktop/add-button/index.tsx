import * as React from 'react'

// Styled Components
import { StyledButton, ButtonText, PlusIcon } from './style'

export interface Props {
  buttonType: 'primary' | 'secondary'
  text: string | undefined
  onSubmit: () => void
  disabled?: boolean
}

export default class AddButton extends React.PureComponent<Props, {}> {
  render () {
    const { onSubmit, text, buttonType, disabled } = this.props
    return (
      <StyledButton disabled={disabled} buttonType={buttonType} onClick={onSubmit}>
        <PlusIcon />
        <ButtonText buttonType={buttonType}>{text}</ButtonText>
      </StyledButton>
    )
  }
}
