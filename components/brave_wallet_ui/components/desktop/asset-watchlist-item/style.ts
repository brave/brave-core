import styled from 'styled-components'

interface StyleProps {
  icon: string
}

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  margin: 8px 0px;
`

export const NameAndIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  width: 42%;
`

export const AssetName = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
`

export const Balance = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
  width: 48%;
`

export const BalanceColumn = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: column;
`

export const AssetIcon = styled.div<StyleProps>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background: ${(p) => p.icon ? `url(${p.icon})` : p.theme.color.background01}};
  margin-right: 8px;
`

export const CheckboxRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-end;
  flex-direction: row;
  width: 10%;
`
