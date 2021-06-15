import styled from 'styled-components'

interface StyleProps {
  labelPosition: 'start' | 'middle' | 'end'
}

export const StyledWrapper = styled.div`
  width: 100%;
  height: 200px;
  margin-bottom: 30px;
`

export const LabelWrapper = styled.div<StyleProps>`
  display: flex;
  align-items: center;
  justify-content: center;
  position: absolute;
  top: -16px;
  transform: ${(p) => p.labelPosition === 'start' ? 'translateX(0%)' : p.labelPosition === 'end' ? 'translateX(-100%)' : 'translateX(-50%)'};
`

export const ChartLabel = styled.span`
  font-family: Poppins;
  font-size: 13px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
`
