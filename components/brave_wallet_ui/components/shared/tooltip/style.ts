import styled from 'styled-components'

interface StyleProps {
  position: 'left' | 'right' | 'center'
  isAddress?: boolean
}

export const StyledWrapper = styled.div`
  display: inline-block;
  position: relative;
  cursor: default;
`

export const Tip = styled.div<StyleProps>`
  position: absolute;
  border-radius: 4px;
  left: ${(p) =>
    p.position === 'right'
      ? 'unset'
      : p.position === 'left'
        ? '0px'
        : '50%'};
  right: ${(p) =>
    p.position === 'right'
      ? '0px'
      : 'unset'};
  transform: ${(p) =>
    p.position === 'center'
      ? 'translateX(-50%)'
      : 'translateX(0)'} translateY(8px);
  padding: 6px;
  color: ${(p) => p.theme.palette.white};
  background: ${(p) => p.theme.palette.black};
  z-index: 100;
  white-space: ${(p) => p.isAddress ? 'pre-line' : 'nowrap'};
  font-family: Poppins;
  font-size: 12px;
  letter-spacing: 0.01em;
  width: ${(p) => p.isAddress ? '180px' : 'unset'};
  word-break: ${(p) => p.isAddress ? 'break-all' : 'unset'};
`

export const Pointer = styled.div<StyleProps>`
  width: 0;
  height: 0;
  border-style: solid;
  position: absolute;
  left: ${(p) =>
    p.position === 'right'
      ? 'unset'
      : p.position === 'left'
        ? '25px'
        : '50%'};
  right: ${(p) =>
    p.position === 'right'
      ? '25px'
      : 'unset'};
  transform: ${(p) =>
    p.position === 'center'
      ? 'translateX(-50%)'
      : 'translateX(0)'} translateY(25%);
  border-width: 0 7px 8px 7px;
  border-color: transparent transparent ${(p) => p.theme.palette.black} transparent;
`

export const ActionNotification = styled(Tip)`
  background: ${p => p.theme.palette.blurple500};
`
