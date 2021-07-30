import styled from 'styled-components'
import { CaratStrongLeftIcon } from 'brave-ui/components/icons'

// Will use brave-ui button comp in the future!
// Currently is missing "tiny" variant
export const StyledWrapper = styled.button`
  display: flex;;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border-radius: 48px;
  padding: 3px 14px;
  border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
  font-family: Poppins;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.interactive07};
`

export const BackIcon = styled(CaratStrongLeftIcon)`
  width: auto;
  height: 14px;
  margin-right: 8px;
  color: ${(p) => p.theme.color.text03};
`
