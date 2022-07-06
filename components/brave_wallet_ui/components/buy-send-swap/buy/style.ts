import styled from 'styled-components'
import RampIcon from '../../../assets/svg-icons/ramp-icon.svg'
import WyreIcon from '../../../assets/svg-icons/wyre-icon.svg'

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
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

export const NetworkNotSupported = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  text-align: center;
  letter-spacing: 0.01em;
  color: ${p => p.theme.color.text02};
  margin: 55px 20px 59px;
  padding: 0;
`
