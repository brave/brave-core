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

export const ToggleBox = styled.button<Props>`
  --bg-color: ${(p) => p.theme.color.disabled};
  position: relative;
  display: flex;
  width: 100px;
  height: 50px;
  border: 0;
  border-radius: 50px;
  background: var(--bg-color);
  margin-bottom: 24px;

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
export const Knob = styled.div<Props>`
  position: absolute;
  top: 5px;
  left: ${(p) => !p.isActive && '5px'};
  right: ${(p) => p.isActive && '5px'};
  width: 40px;
  height: 40px;
  border-radius: 50%;
  background-color: white;
`

// Status indicator
export const StatusBox = styled.div`
  display: flex;
  align-items: center;
  margin-bottom: 24px;
`

export const StatusText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 400;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.04em;
`

export const ActiveIndicator = styled.span`
  width: 6px;
  height: 6px;
  margin-right: 8px;
  border-radius: 50%;
  background: ${(p) => p.theme.color.successBorder};
`

export const InActiveIndicator = styled(ActiveIndicator)`
  background: ${(p) => p.theme.color.disabled};
`

export const FailedIndicator = styled(ActiveIndicator)`
  background: ${(p) => p.theme.color.warn};
`

export const Loader = styled.span`
  width: 12px;
  height: 12px;
  margin-right: 8px;

  svg>path {
    fill: ${(p) => p.theme.color.text03};
  }
`
