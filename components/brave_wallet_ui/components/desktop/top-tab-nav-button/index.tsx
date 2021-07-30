import * as React from 'react'

// Styled Components
import { StyledButton, ButtonText, TabLine } from './style'

export interface Props {
  isSelected: boolean
  text: string
  onSubmit: () => void
}

export default class TopTabNavButton extends React.PureComponent<Props, {}> {
  render () {
    const { onSubmit, text, isSelected } = this.props
    return (
      <StyledButton isSelected={isSelected} onClick={onSubmit}>
        <ButtonText isSelected={isSelected}>{text}</ButtonText>
        <TabLine isSelected={isSelected} />
      </StyledButton>
    )
  }
}
