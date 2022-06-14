import styled from 'styled-components'
import RampIcon from '../../../assets/svg-icons/ramp-icon.svg'
import WyreIcon from '../../../assets/svg-icons/wyre-icon.svg'
import { WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`

export const FaucetWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
`

export const FaucetTitle = styled.span`
  font-family: Poppins;
  font-size: 20px;
  font-weight: 600;
  line-height: 30px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.02em;
`

export const FaucetDescription = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  font-weight: 300;
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 20px;
`

const LogoBase = styled.div`
  width: 19px;
  height: 22px;
  margin-right: 5px;
  background-size: cover;
  background-repeat: no-repeat;
`

export const WyreLogo = styled(LogoBase)`
  width: 19px;
  height:17px;
  margin-right: 5px;
  background-image: url(${WyreIcon});
  background-size: contain;
  background-position: center;
`

export const RampLogo = styled(LogoBase)`
  width: 19px;
  height: 12px;
  margin-right: 5px;
  background-image: url(${RampIcon});
`

export const Spacer = styled.div`
  margin-bottom: 30px;
`

export const ContinueButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  width: 100%;
  border: none;
  color: ${(p) => p.theme.color.text02};
  font-family: Poppins;
  font-weight: 400;
  box-sizing: border-box;
  background-color: ${(p) => p.theme.color.background02};
  border: ${(p) => `1px solid ${p.theme.color.divider01}`};
  padding: 9px 12px;
  border-radius: 12px;
  cursor: ${p => p.disabled ? 'auto' : 'pointer'};
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
`
