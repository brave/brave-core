import styled from 'styled-components'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'
import { StyledButton } from '../../extension/buttons/nav-button/style'
import { WalletButton } from '../../shared/style'

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

export const ArrowButton = styled(WalletButton)`
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

export const SwapNavButton = styled(StyledButton)`
  width: 100%;
`

export const SwapButtonText = styled.span`
  font-size: 13px;
  font-weight: 600;
  line-height: 20px;
  color: ${(p) => p.theme.palette.white};
`

export const SwapButtonLoader = styled(SwapButtonText)`
  width: 15px;
`
