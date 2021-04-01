import * as React from 'react'

// Styled Components
import { StyledButton, ButtonText, ButtonIcon } from './style'

export interface Props {
  isSelected: boolean
  text: string
  onSubmit: () => void
  icon: string
}

export default class SideNavButton extends React.PureComponent<Props, {}> {
  render () {
    const { onSubmit, text, isSelected, icon } = this.props
    return (
      <StyledButton isSelected={isSelected} onClick={onSubmit}>
        <ButtonIcon icon={icon} />
        <ButtonText isSelected={isSelected}>{text}</ButtonText>
      </StyledButton>
    )
  }
}
