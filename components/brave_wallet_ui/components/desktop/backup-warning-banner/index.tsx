import * as React from 'react'
import locale from '../../../constants/locale'
// Styled Components
import {
  StyledWrapper,
  WarningText,
  BannerButton,
  ButtonRow
} from './style'

export interface Props {
  onBackup: () => void
  onDismiss: () => void
}

const BackupWarningBanner = (props: Props) => {
  const { onDismiss, onBackup } = props

  return (
    <StyledWrapper>
      <WarningText>{locale.backupWarningText}</WarningText>
      <ButtonRow>
        <BannerButton onClick={onBackup} buttonType='primary'>{locale.backupButton}</BannerButton>
        <BannerButton onClick={onDismiss} buttonType='secondary'>{locale.dismissButton}</BannerButton>
      </ButtonRow>
    </StyledWrapper>
  )
}

export default BackupWarningBanner
