import styled from 'styled-components'
import Swap from '../assets/swap.svg'
import {
  VerifiedSIcon,
  CaratCircleODownIcon
} from 'brave-ui/components/icons'

interface StyleProps {
  panelBackground: string
  orb: string
}

export const StyledWrapper = styled.div<Partial<StyleProps>>`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  background: ${(p) => p.panelBackground};
`

export const CenterColumn = styled.div`
  flex: 1;
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  padding: 12px 0px 28px;
  max-width: 300px;
`

export const AccountCircle = styled.div<Partial<StyleProps>>`
  width: 54px;
  height: 54px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  border: 2px solid white;
`

export const AccountNameText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
`

export const AccountAddressText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
  font-weight: 300;
`

export const BalanceColumn = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`

export const AssetBalanceText = styled.span`
  font-family: Poppins;
  font-size: 24px;
  line-height: 36px;
  letter-spacing: 0.02em;
  color: ${(p) => p.theme.palette.white};
  font-weight: 600;
`

export const FiatBalanceText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
  font-weight: 300;
`

export const ConnectedIcon = styled(VerifiedSIcon)`
  width: 14px;
  height: 14px;
  margin-right: 8px;
  color: ${(p) => p.theme.palette.white};
`

export const NotConnectedIcon = styled.div`
  width: 14px;
  height: 14px;
  margin-right: 8px;
  border-radius: 24px;
  border: 1px solid rgba(255,255,255,0.5);
`

export const CaratDownIcon = styled(CaratCircleODownIcon)`
  width: 14px;
  height: 14px;
  margin-left: 8px;
`

export const SwapIcon = styled.div`
  width: 14px;
  height: 11.67px;
  background: url(${Swap});
`

// Will use brave-ui button comp in the future!
// Currently is missing "tiny" variant
export const OvalButton = styled.button`
  display: flex;;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border-radius: 48px;
  padding: 3px 14px;
  border: 1px solid rgba(255,255,255,0.5);
  fontSize: 14px;
  color: ${(p) => p.theme.palette.white};
`

export const OvalButtonText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
  font-weight: 600;
`

export const StatusRow = styled.div`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  padding: 0px 12px;
`
