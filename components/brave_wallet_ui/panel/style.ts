import styled from 'styled-components'

interface StyleProps {
  isLonger: boolean
}

export const PanelWrapper = styled.div<StyleProps>`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 320px;
  height: ${(p) => p.isLonger ? '540px' : '400px'};
`

export const WelcomePanelWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 320px;
  height: 250px;
`

export const SendWrapper = styled.div`
  flex: 1;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  overflow-y: scroll;
  overflow-x: hidden;
  position: relative;
  padding: 0px 24px 12px;
  box-sizing: border-box;
`
