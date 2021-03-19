import * as React from 'react'

// Styled Components
import { StyledWrapper, ButtonText } from './style'

export interface Props {
  buttonType: 'primary' | 'secondary'
  text: string
  onSubmit: () => void
}

export default class NavButton extends React.PureComponent<Props, {}> {
  render () {
    const { onSubmit, text, buttonType } = this.props
    return (
      <StyledWrapper buttonType={buttonType} onClick={onSubmit}>
        <ButtonText buttonType={buttonType}>{text}</ButtonText>
      </StyledWrapper>
    )
  }
}
