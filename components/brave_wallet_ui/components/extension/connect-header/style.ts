import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  min-height: 160px;
`

export const URLText = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: normal;
  font-size: 13px;
  line-height: 20px;
  text-align: center;
  letter-spacing: 0.01em;
  margin-bottom: 11px;
`

export const PanelTitle = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 15px;
  line-height: 20px;
  text-align: center;
  letter-spacing: 0.04em;
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
`

export const ProfileCircle = styled.div`
  width: 44px;
  height: 44px;
  border-radius: 100%;
  background-color: ${(p) => p.theme.palette.grey300};
  margin-bottom: 7px;
`
