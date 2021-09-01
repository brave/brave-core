import styled from 'styled-components'

export const CodeSnippet = styled.pre`
  display: block;
  align-items: flex-start;
  justify-content: flex-start;
  tab-size: 0;
  width: 100%;
  background-color: ${(p) => p.theme.color.divider01};
  padding: 14px;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  text-align: left;
  color: ${(p) => p.theme.color.text03};
  border-radius: 4px;
  margin: 0px;
  margin-bottom: 14px;
  white-space: pre-line;
  box-sizing: border-box;
  word-break: break-all;
`

export const HexBlock = styled.div`
  display: flex;
  width: 100%;
  background-color: ${(p) => p.theme.color.divider01};
  padding: 14px;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  text-align: left;
  color: ${(p) => p.theme.color.text03};
  border-radius: 4px;
  word-break: break-all;
  box-sizing: border-box;
`

export const DetailRow = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: row;
  width: 100%;
  margin-bottom: 14px;
`

export const DetailText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
  margin-left: 10px;
`

export const CodeSnippetText = styled.p`
  margin: 0px;
  padding: 0px;
`

export const TransactionText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`
