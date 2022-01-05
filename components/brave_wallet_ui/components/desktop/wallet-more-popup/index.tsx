import * as React from 'react'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  SettingsIcon,
  LockIcon,
  ExplorerIcon,
  BackupIcon
} from './style'

export interface Props {
  onClickSetting?: () => void
  onClickLock?: () => void
  onClickViewOnBlockExplorer?: () => void
  onClickBackup?: () => void
}

const WalletMorePopup = (props: Props) => {
  const {
    onClickLock,
    onClickSetting,
    onClickViewOnBlockExplorer,
    onClickBackup
  } = props

  return (
    <StyledWrapper>
      {onClickLock &&
        <PopupButton onClick={onClickLock}>
          <LockIcon />
          <PopupButtonText>{getLocale('braveWalletWalletPopupLock')}</PopupButtonText>
        </PopupButton>
      }
      {onClickBackup &&
        <PopupButton onClick={onClickBackup}>
          <BackupIcon />
          <PopupButtonText>{getLocale('braveWalletBackupButton')}</PopupButtonText>
        </PopupButton>
      }
      {onClickSetting &&
        <PopupButton onClick={onClickSetting}>
          <SettingsIcon />
          <PopupButtonText>{getLocale('braveWalletWalletPopupSettings')}</PopupButtonText>
        </PopupButton>
      }
      {onClickViewOnBlockExplorer &&
        <PopupButton onClick={onClickViewOnBlockExplorer}>
          <ExplorerIcon />
          <PopupButtonText>{getLocale('braveWalletTransactionExplorer')}</PopupButtonText>
        </PopupButton>
      }
    </StyledWrapper>
  )
}

export default WalletMorePopup
