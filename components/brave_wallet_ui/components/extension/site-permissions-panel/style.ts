import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
`

export const AddressContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
`

export const AddressScrollContainer = styled.div`
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
  max-height: 104px;
  box-sizing: border-box;
`

export const AccountsTitle = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  margin-bottom: 12px;
  color: ${(p) => p.theme.color.text01};
  opacity: 0.7;
  margin-top: 20px;
`
