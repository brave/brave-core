import styled from 'styled-components'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;
  background: linear-gradient(180deg, #FFFFFF 58.56%, #F8F8FF 100%);
  padding: 42px 0;
`

export const PanelTitle = styled.h1`
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 18px;
  font-weight: 600;
  letter-spacing: 0.02em;
  line-height: 2.6;
  margin: 0 13px 0 0;
`

export const Button = styled.button`
  display: flex;
  align-items: center;
  min-width: 260px;
  height: 48px;
  border-radius: 38px;
  border: 0;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  background-color: ${(p) => p.theme.color.background02};
  padding: 0 16px;
`

export const ButtonText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 400;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.04em;
`

export const ToggleBox = styled.div`
  margin-bottom: 15px;
`

export const StatusIndicatorBox = styled.div`
  margin-bottom: 24px;
`
