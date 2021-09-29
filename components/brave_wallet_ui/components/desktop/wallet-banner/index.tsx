import * as React from 'react'
// Styled Components
import {
  StyledWrapper,
  WarningText,
  BannerButton,
  ButtonRow
} from './style'
import { getLocale } from '../../../../common/locale'
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
        <BannerButton onClick={onDismiss} buttonType='secondary'>{getLocale('braveWalletDismissButton')}</BannerButton>
      </ButtonRow>
    </StyledWrapper>
  )
}

export default WalletBanner
