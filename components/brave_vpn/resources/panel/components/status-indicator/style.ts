import styled from 'styled-components'

interface IndicatorProps {
  isActive: boolean
}

export const Box = styled.div`
  display: flex;
  align-items: center;
`

export const Text = styled.span`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 400;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.04em;
`

export const Indicator = styled.span<IndicatorProps>`
  width: 6px;
  height: 6px;
  margin-right: 8px;
  border-radius: 50%;
  background: ${(p) => p.isActive
  ? p.theme.color.successBorder
  : p.theme.color.disabled };
`
