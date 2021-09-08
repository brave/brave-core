import styled from 'styled-components'
import CheckMark from '../../../assets/svg-icons/big-checkmark.svg'
import SwitchDown from '../../../assets/svg-icons/switch-icon.svg'
import { CaratCircleODownIcon } from 'brave-ui/components/icons'

interface StyleProps {
  panelBackground: string
  orb: string
}

export const StyledWrapper = styled.div<Partial<StyleProps>>`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  background: ${(p) => p.panelBackground};
`

export const CenterColumn = styled.div`
  flex: 1;
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  padding: 12px 0px 28px;
  max-width: 300px;
`

export const AccountCircle = styled.button<Partial<StyleProps>>`
  display: flex;
  cursor: pointer;
  width: 54px;
  height: 54px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  border: 2px solid white;
  position: relative;
  box-sizing: border-box;
  margin-bottom: 6px;
`

export const AccountNameText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
`

export const AccountAddressText = styled.button`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
  font-weight: 300;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const BalanceColumn = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`

export const AssetBalanceText = styled.span`
  font-family: Poppins;
  font-size: 24px;
  line-height: 36px;
  letter-spacing: 0.02em;
  color: ${(p) => p.theme.palette.white};
  font-weight: 600;
`

export const FiatBalanceText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
  font-weight: 300;
`

export const NotConnectedIcon = styled.div`
  width: 14px;
  height: 14px;
  margin-right: 8px;
  border-radius: 24px;
  border: 1px solid rgba(255,255,255,0.5);
`

export const CaratDownIcon = styled(CaratCircleODownIcon)`
  width: 14px;
  height: 14px;
  margin-left: 8px;
`

export const OvalButton = styled.button`
  display: flex;;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border-radius: 48px;
  padding: 3px 14px;
  border: 1px solid rgba(255,255,255,0.5);
  fontSize: 14px;
  color: ${(p) => p.theme.palette.white};
`

export const OvalButtonText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
  font-weight: 600;
`

export const StatusRow = styled.div`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  padding: 0px 12px;
`

export const BigCheckMark = styled.div`
  width: 14px;
  height: 14px;
  background-color: ${(p) => p.theme.palette.white};
  -webkit-mask-image: url(${CheckMark});
  mask-image: url(${CheckMark});
  margin-right: 8px;
`

export const SwitchIcon = styled.div`
  width: 14px;
  height: 14px;
  background: url(${SwitchDown});
  position: absolute;
  left: 0px;
  bottom: 0px;
  z-index: 10;
`
