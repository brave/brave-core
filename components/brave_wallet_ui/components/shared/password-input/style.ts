import styled from 'styled-components'
import WarningCircle from '../../../assets/svg-icons/warning-circle-icon.svg'
import EyeOnIcon from '../../../assets/svg-icons/eye-on-icon.svg'
import EyeOffIcon from '../../../assets/svg-icons/eye-off-icon.svg'
import { WalletButton } from '../../shared/style'

interface StyleProps {
  hasError: boolean
  showPassword?: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  width: 100%;
`
export const InputWrapper = styled.div`
  display: flex;
  position: relative;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 10px;
`
export const Input = styled.input<StyleProps>`
  box-sizing: border-box;
  width: 100%;
  outline: none;
  background-image: none;
  background-color: ${(p) => p.hasError ? p.theme.color.errorBackground : p.theme.color.background02};
  box-shadow: none;
  border: ${(p) => p.hasError ? `4px solid ${p.theme.color.errorBorder}` : `1px solid ${p.theme.color.interactive08}`};
  border-radius: 4px;
  font-family: Poppins;
  font-style: normal;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  padding: 10px 10px 10px 10px;
  margin: 0px;
  color: ${(p) => p.theme.color.text01};
  ::placeholder {
    font-family: Poppins;
    font-style: normal;
    font-size: 12px;
    letter-spacing: 0.01em;
    color: ${(p) => p.theme.color.text03};
    font-weight: normal;
  }
  :focus {
      outline: none;
  }
  ::-webkit-inner-spin-button {
      -webkit-appearance: none;
      margin: 0;
  }
  ::-webkit-outer-spin-button {
      -webkit-appearance: none;
      margin: 0;
  }
`

export const ErrorRow = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: row;
  width: 100%;
  margin-bottom: 10px;
`

export const ErrorText = styled.span`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  width: 240px;
  font-family: Poppins;
  font-size: 12px;
  letter-spacing: 0.01em;
  line-height: 18px;
  color: ${(p) => p.theme.color.errorText};
  padding-left: 4px;
`

export const WarningIcon = styled.div`
  width: 14px;
  height: 15px;
  background: url(${WarningCircle});
`

export const ToggleVisibilityButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: absolute;
  right: 10px;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 0px;
  width: 18px;
  height: 18px;
`

export const ToggleVisibilityIcon = styled.div<Partial<StyleProps>>`
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${(p) => p.showPassword ? EyeOffIcon : EyeOnIcon});
  mask-image: url(${(p) => p.showPassword ? EyeOffIcon : EyeOnIcon});
  mask-size: cover;
`
