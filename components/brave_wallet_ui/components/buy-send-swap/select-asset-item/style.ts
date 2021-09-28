import styled from 'styled-components'

interface StyleProps {
  icon: string | undefined
}

export const StyledWrapper = styled.button`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  margin-bottom: 10px;
  padding: 0px;
`

export const AssetAndBalance = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const AssetName = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
`

export const AssetBalance = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`
export const AssetIcon = styled.div<StyleProps>`
  width: 24px;
  height: 24px;
  background: no-repeat ${(p) => `url(${p.icon})`};
  margin-right: 8px;
  background-size: 100%;
`
