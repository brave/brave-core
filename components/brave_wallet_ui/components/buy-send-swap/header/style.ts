import styled from 'styled-components'
import SwitchDown from '../../../assets/svg-icons/switch-icon.svg'
import {
  CaratCircleODownIcon
} from 'brave-ui/components/icons'

interface StyleProps {
  orb: string
}

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 18px;
`

export const NameAndIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const AccountAndAddress = styled.button`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const AccountName = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
`

export const AccountAddress = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
`
export const AccountCircle = styled.button<StyleProps>`
  display: flex;
  cursor: pointer;
  width: 24px;
  height: 24px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin: 0px;
  padding: 0px;
  outline: none;
  border: none;
  margin-right: 8px;
  position: relative;
`

export const CaratDownIcon = styled(CaratCircleODownIcon)`
  width: 14px;
  height: 14px;
  margin-left: 8px;
  color: ${(p) => p.theme.color.interactive07};
`

// Will use brave-ui button comp in the future!
// Currently is missing "tiny" variant
export const OvalButton = styled.button`
  display: flex;;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border-radius: 48px;
  padding: 3px 14px;
  border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
  fontSize: 14px;
`

export const OvalButtonText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  font-weight: 600;
`

export const SwitchIcon = styled.div`
  width: 14px;
  height: 14px;
  background: url(${SwitchDown});
  position: absolute;
  left: -2px;
  bottom: -2px;
  z-index: 8;
`
