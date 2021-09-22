import styled, { css, keyframes } from 'styled-components'
interface Props {
  isActive: boolean
}

const moveBg = keyframes`
  0% {
    background-position: 0% 50%;
  }
  50% {
    background-position: 100% 50%;
  }
  100% {
    background-position: 0% 50%;
  }
`

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
    --bg-color: linear-gradient(
      135deg,
      #381e85 0%,
      #6845d1 30%,
      #737ade 100%,
      #4d56d0 75%,
      #0e1bd1 100%
	  );
    background-size: 400% 400%;
    animation: ${moveBg} 5s ease infinite;
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
