import styled from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'

interface StyleProps {
  labelPosition: 'start' | 'middle' | 'end'
  isLoading: boolean
}

export const StyledWrapper = styled.div`
  width: 100%;
  height: 200px;
  margin-bottom: 30px;
  box-sizing: border-box;
  position: relative;
`

export const LabelWrapper = styled.div<Partial<StyleProps>>`
  display: flex;
  align-items: center;
  justify-content: center;
  position: absolute;
  top: -16px;
  transform: ${(p) => p.labelPosition === 'start' ? 'translateX(0%)' : p.labelPosition === 'end' ? 'translateX(-100%)' : 'translateX(-50%)'};
  white-space: nowrap
`

export const ChartLabel = styled.span`
  font-family: Poppins;
  font-size: 13px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
`

export const LoadingOverlay = styled.div<Partial<StyleProps>>`
  display: ${(p) => p.isLoading ? 'flex' : 'none'};
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  position: absolute;
  z-index: 10;
  backdrop-filter: blur(5px);
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.color.interactive08};
  height: 70px;
  width: 70px;
  opacity: .4;
`
