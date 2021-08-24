import * as React from 'react'
import locale from '../../../constants/locale'
// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  SettingsIcon,
  LockIcon
} from './style'

export interface Props {
  onClickSetting: () => void
  onClickLock: () => void
}

const WalletMorePopup = (props: Props) => {
  const {
    onClickLock,
    onClickSetting
  } = props

  return (
    <StyledWrapper>
      <PopupButton onClick={onClickLock}>
        <LockIcon />
        <PopupButtonText>{locale.walletPopupLock}</PopupButtonText>
      </PopupButton>
      <PopupButton onClick={onClickSetting}>
        <SettingsIcon />
        <PopupButtonText>{locale.walletPopupSettings}</PopupButtonText>
      </PopupButton>
    </StyledWrapper>
  )
}

export default WalletMorePopup
