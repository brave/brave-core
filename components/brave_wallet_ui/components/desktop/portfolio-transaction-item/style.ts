import styled from 'styled-components'
import { TransactionStatus } from '../../../constants/types'
import { MoreVertRIcon, ArrowRightIcon } from 'brave-ui/components/icons'

interface StyleProps {
  orb: string
  status: TransactionStatus
}

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  margin: 14px 0px;
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
  color: ${(p) => p.theme.color.text01};
  margin: 0px 5px;
`

export const DetailText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 400;
  color: ${(p) => p.theme.color.text02};
`

export const FromCircle = styled.div<Partial<StyleProps>>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: 30px;
`

export const ToCircle = styled.div<Partial<StyleProps>>`
  width: 24px;
  height: 24px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  position: absolute;
  left: 26px;
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
  color: ${(p) => p.theme.color.interactive08};
`

export const DetailColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const DetailTextLight = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
  margin-right: 6px;
`

export const DetailTextDark = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  margin-right: 6px;
`

export const DetailTextDarkBold = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text02};
`

export const BalanceColumn = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: column;
`

export const ArrowIcon = styled(ArrowRightIcon)`
  width: auto;
  height: 16px;
  margin-right: 6px;
  color: ${(p) => p.theme.color.text03};
`

export const TransactionDetailRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  width: 50%;
`

export const StatusRow = styled.div`
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
`

export const StatusBubble = styled.div<Partial<StyleProps>>`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 10px;
  height: 10px;
  border-radius: 100%;
  opacity: ${(p) => p.status === 3 || p.status === 1 || p.status === 0 ? 0.4 : 1};
  background-color: ${(p) => p.status === 4 || p.status === 1 ?
    '#2AC194'
    : p.status === 2 || p.status === 5 ? '#EE6374' :
      p.status === 0 ? p.theme.color.interactive08 : p.theme.color.warningIcon
  };
  margin-right: 6px;
`
