import styled from 'styled-components'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`

export const ArrowDownIcon = styled(CaratStrongDownIcon)`
  width: 18px;
  height: auto;
  color: ${(p) => p.theme.color.text02};
`

export const ArrowButton = styled.button`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  width: 48px;
  padding: 0px;
  margin-bottom: 12px;
  border-radius: 4px;
  &:hover {
    background-color: ${(p) => p.theme.color.divider01}
  }
`
