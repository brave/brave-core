import styled from 'styled-components'

interface StyleProps {
  isSelected: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  border: 1px solid #E9E9F4;
  box-sizing: border-box;
  border-radius: 12px;
  padding: 8px;
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
    p.isSelected ? `${p.theme.palette.neutral700}` : `none`};
  border: none;
  margin: 0px 4px;
`

export const ButtonText = styled.span<Partial<StyleProps>>`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) =>
    p.isSelected ? `${p.theme.palette.white}` : `${p.theme.palette.neutral700}`};
`

export const Dot = styled.div<Partial<StyleProps>>`
  width: 6px;
  height: 6px;
  border-radius: 100%;
  margin-right: 6px;
  background-color: ${(p) =>
    p.isSelected ? `${p.theme.palette.white}` : `${p.theme.palette.grey200}`};
`
