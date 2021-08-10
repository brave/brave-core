import styled from 'styled-components'
// import Action from '../assets/actions.svg'

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

export const CenterColumn = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 100%;
`

export const AddressAndOrb = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const AccountCircle = styled.div<Partial<StyleProps>>`
  width: 32px;
  height: 32px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
`

export const AddressText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
  margin-right: 12px;
`

export const NetworkText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

export const PanelTitle = styled.span`
  font-family: Poppins;
  font-size: 15px;
  line-height: 20px;
  letter-spacing: 0.04em;
  text-align: center;
  color: ${(p) => p.theme.color.text01};
  font-weight: 600;
  margin-bottom: 6px;
`

export const Description = styled.span`
  width: 275px;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  text-align: center;
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 12px;
`

export const MessageBoxTitle = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.text01};
  font-weight: 600;
  margin-bottom: 10px;
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
  height: 70px;
  padding: 8px 14px;
  margin-bottom: 14px;
`

export const TransactionText = styled.span`
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

// This is temp, will be replaced with real FavIcon
export const FavIcon = styled.img`
	width: 48px;
	height: 48px;
	border-radius: 5px;
	background-color: ${(p) => p.theme.palette.grey200};
	margin-bottom: 7px;
`

export const MessageBoxRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
`

export const EditButton = styled.button`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
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

export const BalanceText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

export const FiatRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-end;
  flex-direction: row;
  width: 100%;
`

export const FiatBalanceText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`
