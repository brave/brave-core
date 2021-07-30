import styled from 'styled-components'
import { CaratStrongLeftIcon } from 'brave-ui/components/icons'

export const Header = styled.div`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 14px;
`

export const HeaderText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
`

export const BackButton = styled.button`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  width: 16px;
  height: 16px;
  padding: 0px;
`

export const BackIcon = styled(CaratStrongLeftIcon)`
  width: 16px;
  height: 16px;
  color: ${(p) => p.theme.color.text02};
`

export const HeaderSpacing = styled.div`
  width: 16px;
  height: 16px;
`
