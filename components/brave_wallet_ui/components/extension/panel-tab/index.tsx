import * as React from 'react'

// Styled Components
import { StyledButton, ButtonText, TabLine } from './style'

export interface Props {
  isSelected: boolean
  text: string
  onSubmit?: () => void
}

const PanelTab = (props: Props) => {
  const { onSubmit, text, isSelected } = props
  return (
    <StyledButton isSelected={isSelected} onClick={onSubmit}>
      <ButtonText isSelected={isSelected}>{text}</ButtonText>
      <TabLine isSelected={isSelected} />
    </StyledButton>
  )
}

export default PanelTab
