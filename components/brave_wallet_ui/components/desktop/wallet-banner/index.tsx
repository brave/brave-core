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
  onClick: () => void
  onDismiss: () => void
  bannerType: 'warning' | 'danger'
  description: string
  buttonText: string
}

const WalletBanner = (props: Props) => {
  const { onDismiss, onClick, bannerType, description, buttonText } = props

  return (
    <StyledWrapper bannerType={bannerType}>
      <WarningText>{description}</WarningText>
      <ButtonRow>
        <BannerButton onClick={onClick} buttonType='primary'>{buttonText}</BannerButton>
        <BannerButton onClick={onDismiss} buttonType='secondary'>{locale.dismissButton}</BannerButton>
      </ButtonRow>
    </StyledWrapper>
  )
}

export default WalletBanner
