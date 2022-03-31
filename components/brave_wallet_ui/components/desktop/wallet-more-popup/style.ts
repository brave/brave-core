import styled from 'styled-components'
import { SettingsAdvancedIcon, OpenNewIcon } from 'brave-ui/components/icons'
import { LockIconD, SafeIcon, InternetIcon } from '../../../assets/svg-icons/nav-button-icons'
import { WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-conent: center;
  padding: 7px;
  background-color: ${(p) => p.theme.color.background02};
  border-radius: 8px;
  box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.25);
  position: absolute;
  top: 35px;
  right: 15px;
  z-index: 20;
 `

export const PopupButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  text-align: left;
  cursor: pointer;
  width: 220px;
  border-radius: 8px;
  outline: none;
  border: none;
  background: none;
  padding: 10px 0px;
  margin: 0px;
  background-color: transparent;
  &:hover {
    background-color: ${(p) => p.theme.color.divider01};
  }
`

export const PopupButtonText = styled.span`
  flex: 1;
  font-family: Poppins;
  font-size: 13px;
  font-weight: 600;
  letter-spacing: 0.01em;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
`

export const SettingsIcon = styled(SettingsAdvancedIcon)`
  width: 20px;
  height: 20px;
  color: ${(p) => p.theme.color.interactive07};
  margin-right: 18px;
  margin-left: 14px;
`

export const ExplorerIcon = styled(OpenNewIcon)`
  width: 20px;
  height: 20px;
  color: ${(p) => p.theme.color.interactive07};
  margin-right: 18px;
  margin-left: 14px;
`

export const LockIcon = styled.div`
  width: 20px;
  height: 20px;
  margin-right: 18px;
  margin-left: 14px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${LockIconD});
  mask-image: url(${LockIconD});
`

export const BackupIcon = styled.div`
  width: 20px;
  height: 20px;
  margin-right: 18px;
  margin-left: 14px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${SafeIcon});
  mask-image: url(${SafeIcon});
`

export const ConnectedSitesIcon = styled.div`
  width: 20px;
  height: 20px;
  margin-right: 18px;
  margin-left: 14px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${InternetIcon});
  mask-image: url(${InternetIcon});
  mask-size: contain;
`
