import styled from 'styled-components'

interface StyleProps {
  isSelected: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  border: ${(p) => `1px solid ${p.theme.color.divider01}`};
  box-sizing: border-box;
  border-radius: 12px;
  padding: 8px;
  --selected-color: ${p => p.theme.palette.white};
  @media (prefers-color-scheme: dark) {
    --selected-color: ${p => p.theme.color.background02};
  }
`

export const StyledButton = styled.button<Partial<StyleProps>>`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  border-radius: 4px;
  outline: none;
  padding: 4px 6px;
  background: ${(p) =>
    p.isSelected ? p.theme.color.text02 : `none`};
  border: none;
  margin: 0px 4px;
`

export const ButtonText = styled.span<Partial<StyleProps>>`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${p => p.isSelected ? 'var(--selected-color)' : p.theme.color.text02};
`

export const Dot = styled.div<Partial<StyleProps>>`
  width: 6px;
  height: 6px;
  border-radius: 100%;
  margin-right: 6px;
  background-color: ${p => p.isSelected ? 'var(--selected-color)' : p.theme.color.disabled};
`
