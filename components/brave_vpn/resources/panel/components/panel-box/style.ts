import styled from 'styled-components'
import wavesLightUrl from '../../assets/svg-icons/waves-light.svg'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  max-height: 450px;
  background: ${(p) => p.theme.color.panelBackground};
  position: relative;
  overflow: hidden;
`
export const Waves = styled.div`
  width: 100%;
  height: 100%;
  background-image: url(${wavesLightUrl});
  background-repeat: no-repeat;
  background-size: cover;
  background-position: top;
  position: absolute;
  bottom: 0;
  z-index: 1;
  user-select: none;
  pointer-events: none;
`
