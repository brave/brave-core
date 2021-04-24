import styled from 'styled-components'

interface StyleProps {
  icon: string
}

export const StyledWrapper = styled.button`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  margin: 8px 0px;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const NameAndIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const AssetName = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.neutral900};
`

export const BalanceColumn = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: column;
`

export const FiatBalanceText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.neutral900};
`

export const AssetBalanceText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.neutral600};
`

export const AssetIcon = styled.div<StyleProps>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background: ${(p) => `url(${p.icon})`};
  margin-right: 8px;
`
