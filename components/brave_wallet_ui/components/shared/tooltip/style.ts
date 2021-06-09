import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: inline-block;
  position: relative;
`

export const Tip = styled.div`
  position: absolute;
  border-radius: 4px;
  left: 50%;
  transform: translateX(-50%) translateY(25%);
  padding: 6px;
  color: ${(p) => p.theme.palette.white};
  background: ${(p) => p.theme.palette.black};
  z-index: 100;
  white-space: nowrap;
  font-family: Poppins;
  font-size: 12px;
  letter-spacing: 0.01em;
`

export const Pointer = styled.div`
  width: 0;
  height: 0;
  border-style: solid;
  position: absolute;
  left: 50%;
  transform: translateX(-50%) translateY(25%);
  border-width: 0 7px 8px 7px;
  border-color: transparent transparent ${(p) => p.theme.palette.black} transparent;
`
