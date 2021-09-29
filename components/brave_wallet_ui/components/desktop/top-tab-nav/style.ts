import styled from 'styled-components'
import { MoreVertRIcon } from 'brave-ui/components/icons'
import { LockIconD } from '../../../assets/svg-icons/nav-button-icons'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  margin-bottom: 20px;
`

export const MoreRow = styled.div`
  flex: 1;
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: column;
  padding: 10px 0px 0px 0px;
`

export const Line = styled.div`
  display: flex;
  width: 100%;
  height: 2px;
  background: ${(p) => p.theme.color.divider01};
`

export const MoreButton = styled.button`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 20px;
  height: 20px;
  margin-bottom: 10px;
  margin-right: 10px;
  outline: none;
  border: none;
  background: none;
  padding: 0px;
`

export const EmptyPadding = styled.div`
  width: 20px;
  height: 20px;
  margin-bottom: 10px;
`

export const MoreIcon = styled(MoreVertRIcon)`
  width: 20px;
  transform: rotate(90deg);
  height: 20px;
  color: ${(p) => p.theme.color.interactive07};
`

export const LockIcon = styled.div`
  width: 20px;
  height: 20px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${LockIconD});
  mask-image: url(${LockIconD});
`
