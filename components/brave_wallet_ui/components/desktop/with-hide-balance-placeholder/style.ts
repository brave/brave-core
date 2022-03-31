import styled from 'styled-components'

interface StyleProps {
  isBig: boolean
}

export const PlaceholderText = styled.span<StyleProps>`
  font-family: Poppins;
  font-size: ${(p) => p.isBig ? '32px' : '14px'};
  line-height: ${(p) => p.isBig ? '32px' : '20px'};
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
  height: ${(p) => p.isBig ? '32px' : 'auto'};
  margin-bottom: ${(p) => p.isBig ? '20px' : '0px'};
`
