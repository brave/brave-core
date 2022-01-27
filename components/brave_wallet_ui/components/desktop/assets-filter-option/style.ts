import styled from 'styled-components'

interface StyleProps {
  selected?: boolean
}

export const Option = styled.li<Partial<StyleProps>>`
  display: flex;
  align-items: center;
  padding: 10px 0;
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  font-weight: ${(p) => p.selected ? 600 : 'normal'};
  cursor: pointer;
`
