import styled from 'styled-components'

interface StyleProps {
  buttonType: 'primary' | 'secondary'
}

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  background-color: ${(p) => p.theme.color.warningBackground};
  border-radius: 4px;
  border: 1px solid ${(p) => p.theme.color.warningBorder};
  padding: 20px;
  margin-bottom: 14px;
 `

export const WarningText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 500;
  line-height: 18px;
  color: ${(p) => p.theme.color.text01};
`

export const ButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
`

export const BannerButton = styled.button<StyleProps>`
  display: flex;;
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 0px;
  margin: 0px;
  font-family: Poppins;
  font-size: 12px;
  font-weight: 600;
  color: ${(p) => p.buttonType === 'primary' ? p.theme.color.interactive05 : p.theme.color.text02};
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.buttonType === 'primary' ? p.theme.palette.white : p.theme.color.text02};
  }
  letter-spacing: 0.01em;
  margin-left: 20px;
`
