import styled from 'styled-components'

interface StyleProps {
  textSize?: 'large' | 'normal'
}

export const StyledWrapper = styled.div`
  width: 332px;
  height: 232px;
  font-family: Poppins, sans-serif;
  font-style: normal;
  background: #ffffff;
`

export const StyledCaptchaFrame = styled.iframe`
  width: 323px;
  height: 275px;
  font-family: Poppins, sans-serif;
  font-style: normal;
  background: #ffffff;
  border: 0;
  margin-right: 4px;
`

export const StyledIcon = styled.img`
  width: 24px;
  height: 24px;
  margin-left: 25px;
  margin-right: 8px;
  float: left;
`

export const StyledValidationSpinner = styled.img`
  position: absolute;
  left: 39.16%;
  right: 39.16%;
  top: 30.67%;
  bottom: 41.53%;
`

export const StyledValidationText = styled.p`
  position: absolute;
  width: 93px;
  height: 24px;
  left: 119.5px;
  top: 144px;
  font-style: normal;
  font-weight: normal;
  font-size: 16px;
  line-height: 24px;
  text-align: center;
  color: #495057;
`

export const StyledTitle = styled.h1<StyleProps>`
  margin-top: 50px;
  margin-bottom: 5px;
  width: 290px;
  font-style: normal;
  font-weight: 600;
  font-size: ${p => p.textSize === 'large' ? '22px' : '18px'};
  color: #212529;
`

export const StyledText = styled.p`
  margin-left: 25px;
  margin-right: 16px;
  margin-top: 0;
  font-style: normal;
  font-weight: 400;
  font-size: 16px;
  line-height: 24px;
  color: #212529;
`

export const StyledBorderlessButton = styled.button`
  display: block;
  height: 40px;
  margin: 0 auto;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  background-color: #ffffff;
  color: #212529;
  border: 0;
`

export const StyledButton = styled.button`
  display: block;
  height: 40px;
  margin: 0 auto;
  padding: 10px 22px;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  background-color: #ffffff;
  color: #212529;
  border: 1px solid #aeb1c2;
  box-sizing: border-box;
  border-radius: 48px;
`
