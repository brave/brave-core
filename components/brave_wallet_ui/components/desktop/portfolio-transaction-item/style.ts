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
  position: relative;
`

export const DetailRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const AddressText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.palette.neutral900};
  margin: 0px 5px;
`

export const DetailText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 400;
  color: ${(p) => p.theme.palette.neutral700};
`

export const FromCircle = styled.div<StyleProps>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: 30px;
`

export const ToCircle = styled.div<StyleProps>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  position: absolute;
  left: 23px;
`

export const MoreButton = styled.button`
  display: flex;
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
