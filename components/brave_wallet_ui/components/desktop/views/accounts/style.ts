import styled from 'styled-components'
import { EditOIcon, WalletAddIcon } from 'brave-ui/components/icons'
import FlashdriveIcon from '../../../../assets/svg-icons/flashdrive-icon.svg'
import QRICON from '../../../../assets/svg-icons/qr-code-icon.svg'
import { WalletButton } from '../../../shared/style'

interface StyleProps {
  isHardwareWallet: boolean
  orb: string
}

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  margin-bottom: 40px;
`

export const PrimaryListContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  background-color: ${(p) => p.theme.color.divider01};
  border-radius: 16px;
  margin-top: 14px;
  margin-bottom: 14px;
  padding: 15px 15px 0px 15px;
`

export const SecondaryListContainer = styled.div<Partial<StyleProps>>`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  background-color: ${(p) => p.isHardwareWallet ? p.theme.color.divider01 : 'transparent'};
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
  margin-top: 10px;
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
  margin-bottom: 14px;
  margin-top: 6px;
  color: ${(p) => p.theme.color.text03};
`

export const SubDivider = styled.div`
  width: 100%;
  height: 2px;
  background-color: ${(p) => p.theme.color.divider01};
`

export const Button = styled(WalletButton)`
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

export const TopRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
`

export const WalletInfoRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  margin: 30px 0px;
`

export const WalletInfoLeftSide = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
`

export const AccountCircle = styled.div<Partial<StyleProps>>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: 14px;
`

export const WalletName = styled.span`
  font-family: Poppins;
  font-size: 20px;
  line-height: 30px;
  font-weight: 600;
  letter-spacing: 0.02em;
  color: ${(p) => p.theme.color.text02};
  margin-right: 15px;
`

export const WalletAddress = styled(WalletButton)`
  font-family: Poppins;
  font-size: 15px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.04em;
  color: ${(p) => p.theme.color.text03};
  margin-right: 15px;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const QRCodeIcon = styled.div`
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${QRICON});
  mask-image: url(${QRICON});
`

export const EditIcon = styled(EditOIcon)`
  width: 18px;
  height: 18px;
  color: ${(p) => p.theme.color.interactive07};
`

export const SubviewSectionTitle = styled.span`
  font-family: Poppins;
  font-size: 15px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.04em;
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 10px;
`

export const TransactionPlaceholderContainer = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  height: 100px;
  padding-top: 10px;
`

export const ButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  margin-bottom: 20px;
`

export const StyledButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  border-radius: 40px;
  padding: 12px 22px;
  outline: none;
  background-color: transparent;
  border: 1px solid ${(p) => p.theme.color.interactive08};
  margin-right: 8px;
`

export const ButtonText = styled.span`
  font-size: 13px;
  font-weight: 600;
  color: ${(p) => p.theme.color.interactive07};
`

export const WalletIcon = styled(WalletAddIcon)`
  width: 15px;
  height: 15px;
  color: ${(p) => p.theme.color.text02};
  margin-right: 8px;
`

export const HardwareIcon = styled.div`
  width: 15px;
  height: 15px;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${FlashdriveIcon});
  mask-image: url(${FlashdriveIcon});
  mask-size: cover;
  margin-right: 8px;
`
