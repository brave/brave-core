import * as React from 'react'
import locale from '../../../constants/locale'
// Styled Components
import {
  StyledWrapper,
  WarningText,
  DismissButton
} from './style'

export interface Props {
  onDismiss: () => void
}

const BackupWarningBanner = (props: Props) => {
  const { onDismiss } = props

  return (
    <StyledWrapper>
      <WarningText>{locale.backupWarningText}</WarningText>
      <DismissButton onClick={onDismiss}>Dismiss</DismissButton>
    </StyledWrapper>
  )
}

export default BackupWarningBanner
