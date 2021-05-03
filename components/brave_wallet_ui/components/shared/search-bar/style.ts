import styled from 'styled-components'
import Icon from '../../../assets/svg-icons/search-icon.svg'

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  height: 32px;
  border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
  box-sizing: border-box;
  border-radius: 8px;
  background-color: ${(p) => p.theme.color.background02};
  margin-bottom: 10px;
  overflow: hidden;
`

export const SearchInput = styled.input`
  flex: 1;
  height: 100%;
  outline: none;
  background-image: none;
  background-color: ${(p) => p.theme.color.background02};
  box-shadow: none;
  border: none;
  font-family: Poppins;
  font-style: normal;
  font-size: 12px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
  -webkit-box-shadow: none;
  -moz-box-shadow: none;
  ::placeholder {
    font-family: Poppins;
    font-style: normal;
    font-size: 12px;
    letter-spacing: 0.01em;
    color: ${(p) => p.theme.color.text02};
    font-weight: normal;
  }
  :focus {
      outline: none;
  }
  ::-webkit-inner-spin-button {
      -webkit-appearance: none;
      margin: 0;
  }
  ::-webkit-outer-spin-button {
      -webkit-appearance: none;
      margin: 0;
  }
`

export const SearchIcon = styled.div`
  width: 15px;
  height: 15px;
  background: url(${Icon});
  margin-left: 10px;
  margin-right: 5px;
`
