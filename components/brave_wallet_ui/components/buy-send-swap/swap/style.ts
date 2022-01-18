import styled from 'styled-components'
import { CaratStrongDownIcon, AlertCircleIcon } from 'brave-ui/components/icons'
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

export const SwapDisclaimerRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  margin-top: 8px;
`

export const SwapDisclaimerText = styled.span`
  font-family: Poppins;
  letter-spacing: 0.01em;
  font-size: 12px;
  line-height: 16px;
  color: ${(p) => p.theme.color.text02};
  word-break: break-word;
  margin-right: 6px;
`

export const SwapDisclaimerButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-size: 12px;
  line-height: 16px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.interactive05};
  margin: 0px;
  padding: 0px;
`

export const AlertIcon = styled(AlertCircleIcon)`
  width: 14px;
  height: 14px;
  color: ${(p) => p.theme.color.interactive05};
`

export const SwapFeesNoticeRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  margin-top: -4px;
  margin-bottom: 8px;
`

export const SwapFeesNoticeText = styled.div`
  font-family: Poppins;
  letter-spacing: 0.01em;
  font-size: 12px;
  line-height: 16px;
  color: ${(p) => p.theme.color.text02};
  word-break: break-word;
`

export const ResetRow = styled.div`
  display: flex;
  justify-content: flex-end;
  align-items: center;
  width: 100%;
  margin-bottom: 12px;
`
