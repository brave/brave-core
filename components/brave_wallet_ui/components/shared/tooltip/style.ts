import styled from 'styled-components'

interface StyleProps {
  positionRight: boolean
}

export const StyledWrapper = styled.div`
  display: inline-block;
  position: relative;
`

export const Tip = styled.div<StyleProps>`
  position: absolute;
  border-radius: 4px;
  left: ${(p) => p.positionRight ? 'none' : '50%'};
  right: ${(p) => p.positionRight ? '0px' : 'none'};
  transform: ${(p) => p.positionRight ? 'translateX(0)' : 'translateX(-50%)'} translateY(25%);
  padding: 6px;
  color: ${(p) => p.theme.palette.white};
  background: ${(p) => p.theme.palette.black};
  z-index: 100;
  white-space: nowrap;
  font-family: Poppins;
  font-size: 12px;
  letter-spacing: 0.01em;
`

export const Pointer = styled.div<StyleProps>`
  width: 0;
  height: 0;
  border-style: solid;
  position: absolute;
  left: ${(p) => p.positionRight ? 'none' : '50%'};
  right: ${(p) => p.positionRight ? '25px' : 'none'};
  transform: ${(p) => p.positionRight ? 'translateX(0)' : 'translateX(-50%)'} translateY(25%);
  border-width: 0 7px 8px 7px;
  border-color: transparent transparent ${(p) => p.theme.palette.black} transparent;
`
