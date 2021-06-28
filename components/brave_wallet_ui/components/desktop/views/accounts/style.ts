import styled from 'styled-components'
import { SettingsAdvancedIcon } from 'brave-ui/components/icons'
import { SafeIcon } from '../../../../assets/svg-icons/nav-button-icons'

interface StyleProps {
  isHardwareWallet: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  margin-top: 20px;
`

export const PrimaryRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: space-between;
  width: 100%;
`

export const PrimaryListContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  background-color: ${(p) => p.theme.color.warningBackground};
  border-radius: 16px;
  margin-top: 20px;
  margin-bottom: 30px;
  padding: 15px 15px 0px 15px;
`

export const SecondaryListContainer = styled.div<StyleProps>`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  padding: 15px 15px 0px 15px;
  background-color: ${(p) => p.isHardwareWallet ? p.theme.color.infoBackground : 'transparent'};
  border-radius: 16px;
  padding: 15px 15px 0px 15px;
  margin-bottom: ${(p) => p.isHardwareWallet ? '15px' : '0px'};
`

export const SectionTitle = styled.span`
  font-family: Poppins;
  font-size: 15px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.04em;
  color: ${(p) => p.theme.color.text02};
`

export const DisclaimerText = styled.span`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: row;
  max-width: 760px;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  text-align: flex-start;
  margin-bottom: 10px;
  margin-top: 6px;
  color: ${(p) => p.theme.color.text03};
`

export const SubDivider = styled.div`
  width: 100%;
  height: 2px;
  background-color: ${(p) => p.theme.color.divider01};
`

export const ButtonsRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  padding-right: 20px;
`

export const BackupButton = styled.button`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 0px;
  margin-right: 45px;
`

export const BackupButtonText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
`

export const SettingsButton = styled.button`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 0px;
`

export const SettingsIcon = styled(SettingsAdvancedIcon)`
  width: 20px;
  height: 20px;
  color: ${(p) => p.theme.color.interactive07};
`

export const BackupIcon = styled.div`
  width: 20px;
  height: 20px;
  margin-right: 8px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${SafeIcon});
  mask-image: url(${SafeIcon});
`
