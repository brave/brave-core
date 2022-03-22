import styled from 'styled-components'
import { WalletButton } from '../../shared/style'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  position: relative;
`

export const NetworkButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  background-color: ${(p) => p.theme.color.background02};
  width: 265px;
  cursor: pointer;
  outline: none;
  background: none;
  border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
  border-radius: 4px;
  font-family: Poppins;
  font-style: normal;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  padding: 10px;
  margin-bottom: 8px;
  color: ${(p) => p.theme.color.text01};
`

export const DropDownIcon = styled(CaratStrongDownIcon)`
  width: 18px;
  height: 18px;
  color: ${(p) => p.theme.color.interactive07};
`

export const DropDown = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-conent: center;
  min-width: 275px;
  max-height: 262px;
  padding: 10px 10px 10px 20px;
  background-color: ${(p) => p.theme.color.background02};
  border: 1px solid ${(p) => p.theme.color.divider01};
  border-radius: 8px;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  @media (prefers-color-scheme: dark) {
    box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
  position: absolute;
  top: 44px;
  left: 0px;
  z-index: 12;
  overflow-y: scroll;
  overflow-x: hidden;
 `
