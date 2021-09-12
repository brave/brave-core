import styled from 'styled-components'
import CheckIcon from '../assets/filled-checkmark.svg'

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  background-color: ${(p) => p.theme.color.background01};
`

export const SelectAddressContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
`

export const SelectAddressScrollContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  padding-top: 8px;
  padding-left: 10px;
  padding-right: 10px;
  overflow-y: scroll;
  overflow-x: hidden;
  position: relative;
  max-height: 200px;
  box-sizing: border-box;
`

export const MiddleWrapper = styled.div`
	display: flex;
	flex-direction: column;
	align-items: center;
	justify-content: space-between;
	width: 100%;
	height: 100%;
`

export const NewAccountTitle = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  padding-left: 12px;
  padding-bottom: 8px;
  align-self: flex-start;
  color: ${(p) => p.theme.color.text02};
`

export const AccountListWrapper = styled.div`
	display: flex;
	flex-direction: column;
	align-items: center;
	justify-content: center;
	width: 100%;
`

export const ConfirmTextRow = styled.div`
	display: flex;
	flex-direction: row;
	align-items: center;
	justify-content: center;
	width: 100%;
	height: 100%;
`

export const ConfirmTextColumn = styled.div`
	display: flex;
	flex-direction: column;
	align-items: flex-start;
	justify-content: center;
`

export const ConfirmText = styled.span`
	font-family: Poppins;
	font-style: normal;
	font-weight: normal;
	font-size: 13px;
	line-height: 20px;
	letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
`

export const Details = styled.span`
	font-family: Poppins;
	font-style: normal;
	font-weight: normal;
	font-size: 11px;
	line-height: 17px;
	text-align: center;
	letter-spacing: 0.01em;
	opacity: 0.7;
	word-wrap: wrap;
	max-width: 75%;
  color: ${(p) => p.theme.color.text01};
`

export const ConfirmIcon = styled.div`
	width: 24px;
	height: 24px;
	background: url(${CheckIcon});
	margin-right: 12px;
	fill: blue;
`
