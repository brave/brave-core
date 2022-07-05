import styled from 'styled-components'
import More from '../../extension/assets/actions.svg'
import { WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  margin-bottom: 24px;
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

export const MoreButton = styled(WalletButton)`
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

export const MoreIcon = styled.div`
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${More});
  mask-image: url(${More});
  mask-size: cover;
`
