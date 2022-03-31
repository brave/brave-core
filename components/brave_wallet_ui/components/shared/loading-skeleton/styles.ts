import styled, { keyframes } from 'styled-components'

export interface LoadingSkeletonStyleProps {
  width?: string | number
  height?: string | number
  borderRadius?: string | number
  inline?: boolean
  duration?: number
  direction?: 'ltr' | 'rtl'
  enableAnimation?: boolean
  circle?: boolean
}

const loadingAnimation = keyframes`
  0% {
    transform: translateX(-100%);
  }
  60% {
    transform: translateX(100%);
  }
  100% {
    transform: translateX(100%);
  }
`

export const Skeleton = styled.span<Partial<LoadingSkeletonStyleProps>>`
  background-color: ${p => p.theme.color.panelBackgroundSecondary};
  width: ${(p) => typeof (p.width) === 'number' ? `${p.width}px` : p.width};
  height: ${(p) => typeof (p.height) === 'number' ? `${p.height}px` : p.height};
  border-radius: ${p => p.circle ? '50%' : '0.25rem'};
  display: inline-flex;
  line-height: 1;
  position: relative;
  overflow: hidden;
  z-index: 1;

  @media (prefers-color-scheme: dark) {
    background-color: ${p => p.theme.color.divider01};
  }
  
  &:after {
    content: ' ';
    display: ${p => p.enableAnimation ? 'block' : 'none'};
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    height: 100%;
    background-repeat: no-repeat;
    background-image: linear-gradient(90deg, transparent, #ededed, transparent);
    transform: translateX(-100%);
    animation-name: ${loadingAnimation};
    animation-direction: ${p => p.direction === 'rtl' ? 'reverse' : 'normal'};
    animation-duration: ${p => p.duration ? p.duration : '2s'};
    animation-timing-function: ease-in-out;
    animation-iteration-count: infinite;
    
    @media (prefers-color-scheme: dark) {
      background-image: linear-gradient(90deg, transparent, #30303d, transparent);
    }
  }
`

export const LineBreak = styled.br`
  content: ''
`

Skeleton.defaultProps = {
  width: '100%',
  height: '100%'
}
