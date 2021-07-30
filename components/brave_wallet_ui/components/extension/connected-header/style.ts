import styled from 'styled-components'
import Exand from '../assets/expand.svg'
import Action from '../assets/actions.svg'

export const HeaderTitle = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
`

export const HeaderWrapper = styled.div`
  display: flex;
  height: 54px;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  border-bottom: 1px solid rgba(255,255,255,0.2);
  padding: 0px 10px;
  max-width: 300px;
`

export const ExpandIcon = styled.button`
  display: flex;;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 20px;
  height: 20px;
  background: url(${Exand});
  outline: none;
  border: none;
`

export const ActionIcon = styled.button`
  display: flex;;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 20px;
  height: 20px;
  background: url(${Action});
  outline: none;
  border: none;
`
