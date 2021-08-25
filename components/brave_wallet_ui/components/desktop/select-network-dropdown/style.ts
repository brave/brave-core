import styled from 'styled-components'
import {
  CaratCircleODownIcon
} from 'brave-ui/components/icons'

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  margin-left: 8px;
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

export const DropDown = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-conent: center;
  width: 225px;
  padding: 0px 0px 0px 10px;
  background-color: ${(p) => p.theme.color.background02};
  border-radius: 8px;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  @media (prefers-color-scheme: dark) {
    box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
  border: 1px solid ${(p) => p.theme.color.divider01};
  position: absolute;
  top: 30px;
  left: 0px;
  z-index: 10;
 `
