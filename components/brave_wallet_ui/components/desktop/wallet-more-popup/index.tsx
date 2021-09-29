import * as React from 'react'
import { getLocale } from '../../../../common/locale'
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
        <PopupButtonText>{getLocale('braveWalletWalletPopupLock')}</PopupButtonText>
      </PopupButton>
      <PopupButton onClick={onClickSetting}>
        <SettingsIcon />
        <PopupButtonText>{getLocale('braveWalletWalletPopupSettings')}</PopupButtonText>
      </PopupButton>
    </StyledWrapper>
  )
}

export default WalletMorePopup
