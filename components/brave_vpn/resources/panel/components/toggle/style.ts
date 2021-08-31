import styled, { css } from 'styled-components'
interface Props {
  isActive: boolean
}

export const Box = styled.button<Props>`
  --bg-color: ${(p) => p.theme.color.disabled};
  position: relative;
  display: flex;
  width: 100px;
  height: 50px;
  border: 0;
  border-radius: 50px;
  background: var(--bg-color);

  ${p => p.isActive && css`
    --bg-color: linear-gradient(90deg, #381E85 0.64%, #6845D1 99.36%)
  `}
`
export const Circle = styled.div<Props>`
  position: absolute;
  top: 5px;
  left: ${(p) => !p.isActive && '5px'};
  right: ${(p) => p.isActive && '5px'};
  width: 40px;
  height: 40px;
  border-radius: 50%;
  background-color: white;
`
