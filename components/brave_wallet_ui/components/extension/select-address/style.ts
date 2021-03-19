import styled from 'styled-components'

interface StyleProps {
  isSelected: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  cursor: pointer;
  padding: 0px 12px;
  padding-bottom: 8px;
`

export const NameAndAddressColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  margin-left: 12px;
`

export const LeftSide = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
`
// Need to add blockies package
export const AccountCircle = styled.div`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background-color: ${(p) => p.theme.palette.blurple500};
`

export const AccountNameText = styled.span`
  font-size: 13px;
  line-height: 20px;
  font-weight: 600;
`

export const AccountAddressText = styled.span`
  font-size: 12px;
  line-height: 18px;
  font-weight: 400;
`
// Need to add Checkmark from brave-ui
export const SelectedCircle = styled.div<StyleProps>`
  width: 24px;
  height: 24px;
  border-radius: 24px;
  background-color: ${(p) => (p.isSelected ? '#12a378' : 'none')};
  border: ${(p) => p.isSelected ? 'none' : `1px solid ${p.theme.color.inputBorder}`};
`
