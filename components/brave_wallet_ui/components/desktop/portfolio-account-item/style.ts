import styled from 'styled-components'
import { MoreVertRIcon } from 'brave-ui/components/icons'

interface StyleProps {
  orb: string
}

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  margin: 10px 0px;
`

export const NameAndIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const AccountAndAddress = styled.button`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const AccountName = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.palette.neutral900};
`

export const AccountAddress = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.neutral700};
`

export const BalanceColumn = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: column;
  margin-right: 20px;
`

export const RightSide = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: row;
`

export const FiatBalanceText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.neutral900};
`

export const AssetBalanceText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.neutral600};
`

export const AccountCircle = styled.div<StyleProps>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: 12px;
`

export const MoreButton = styled.button`
  display: flex;;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const MoreIcon = styled(MoreVertRIcon)`
  width: auto;
  height: 26px;
  transform: rotate(90deg);
`
