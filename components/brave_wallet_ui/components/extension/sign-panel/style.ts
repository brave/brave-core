import styled from 'styled-components'
import Action from '../assets/actions.svg'

interface StyleProps {
  orb: string
}

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  background-color: ${(p) => p.theme.color.background01};
`

export const TopRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  padding: 15px 15px 0px 15px;
`

export const AccountCircle = styled.div<Partial<StyleProps>>`
  width: 54px;
  height: 54px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-bottom: 13px;
`

export const AccountNameText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.01em;
  line-height: 20px;
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 2px;
`

export const NetworkText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

export const PanelTitle = styled.span`
  width: 236px;
  font-family: Poppins;
  font-size: 18px;
  line-height: 26px;
  letter-spacing: 0.02em;
  text-align: center;
  color: ${(p) => p.theme.color.text01};
  font-weight: 600;
  margin-bottom: 15px;
`

export const MessageBox = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  border: 1px solid ${(p) => p.theme.color.divider01};
  box-sizing: border-box;
  border-radius: 4px;
  width: 255px;
  height: 140px;
  padding: 8px 14px;
  margin-bottom: 14px;
`

export const MessageTitle = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  font-weight: 600;
  margin-bottom: 5px;
`

export const MessageText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  text-align: left;
  color: ${(p) => p.theme.color.text02};
`

export const ButtonRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  width: 100%;
  margin-bottom: 14px;
`

export const MoreButton = styled.button`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 20px;
  height: 20px;
  background-color: ${(p) => p.theme.color.text03};
  -webkit-mask-image: url(${Action});
  mask-image: url(${Action});
  outline: none;
  border: none;
`
