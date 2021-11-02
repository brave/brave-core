import styled from 'styled-components'

export const MessageBox = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  border: 1px solid ${(p) => p.theme.color.divider01};
  box-sizing: border-box;
  border-radius: 4px;
  width: 255px;
  height: 136px;
  padding: 8px 14px;
  margin-bottom: 14px;
  overflow-y: auto;
  overflow-x: hidden;
  position: relative;
`

export const NetworkTitle = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  font-weight: 600;
`

export const ButtonRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  width: 100%;
  margin-bottom: 14px;
`

export const URLText = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: normal;
  font-size: 13px;
  line-height: 20px;
  text-align: center;
  letter-spacing: 0.01em;
  margin-bottom: 6px;
  color: ${(p) => p.theme.color.text02};
`

export const FavIcon = styled.img`
  width: 48px;
  height: 48px;
  border-radius: 5px;
  background-color: ${(p) => p.theme.palette.grey200};
  margin-bottom: 7px;
`

export const MessageBoxColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  width: 100%;
  margin-bottom: 6px;
`

export const DetailsButton = styled.button`
  font-family: Poppins;
  font-style: normal;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.interactive05};
  background: none;
  cursor: pointer;
  outline: none;
  border: none;
  margin: 0px;
  padding: 0px;
`

export const NetworkDetail = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

export const TabRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  width: 255px;
  margin-bottom: 10px;
`
