import styled from 'styled-components'

export const Box = styled.div`
  border: 1px solid ${p => p.theme.color.divider01};
  border-radius: 2000px;
  padding: 3px;
  display: flex;
  align-items: center;
  justify-content: center;
  text-align: center;
  position: relative;
  width: 100%;
  height: 48px;
`

export const ButtonLeft = styled.button`
  width: 48px;
  height: 100%;
  border-radius: 100px;
  border: 0;
  background: transparent;
  cursor: pointer;
  position: absolute;
  left: 0;
`

export const ButtonRight = styled(ButtonLeft)`
  left: unset;
  right: 0;
`
