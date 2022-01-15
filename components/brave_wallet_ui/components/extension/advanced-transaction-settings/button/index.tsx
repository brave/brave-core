import * as React from 'react'

// Styled Components
import { StyledButton, SettingsIcon, TabLine } from './style'

export interface Props {
  onSubmit?: () => void
}

const AdvancedTransactionSettingsButton = (props: Props) => {
  const { onSubmit } = props
  return (
    <StyledButton onClick={onSubmit}>
      <SettingsIcon />
      <TabLine />
    </StyledButton>
  )
}

export default AdvancedTransactionSettingsButton
