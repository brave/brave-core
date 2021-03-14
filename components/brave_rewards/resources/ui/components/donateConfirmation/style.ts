import styled from 'styled-components'
import oneTimeBgUrl from './assets/one-time-bg.svg'
import monthlyBgUrl from './assets/monthly-bg.svg'
import palette from 'brave-ui/theme/colors'

interface StyleProps {
  isMonthly?: boolean
}

export const StyledWrapper = styled('div')<StyleProps>`
  height: 100%;
  color: #fff;
  min-height: 240px;
  text-align: center;
  position: relative;
  font-family: Poppins, sans-serif;
  background: url(${(p) => {
    if (p.isMonthly) {
      return monthlyBgUrl
    }
    return oneTimeBgUrl
  }});
  background-repeat: no-repeat;
  background-position: bottom;
`

export const StyledTitle = styled('span')<{}>`
  display: block;
  font-size: 16px;
  font-weight: 600;
  letter-spacing: 0px;
  margin-top: 15px;
`

export const StyledSubTitle = styled('span')<{}>`
  display: block;
  font-size: 12px;
  font-weight: 400;
  letter-spacing: 0px;
  margin-top: 10px;
`

export const StyledTweet = styled('div')<{}>`
  border-radius: 50px;
  width: 96px;
  height: 96px;
  margin: 25px auto 0 auto;
  background: ${palette.blue400};
  cursor: pointer;
`

export const StyledTwitterIcon = styled('div')<{}>`
  height: 80px;
  padding-top: 16px;
  color: #fff;
`

export const StyledInfo = styled('div')<{}>`
  margin: 10px 0;
`

export const StyledInfoItem = styled('span')<{}>`
  display: block;
  margin-bottom: 5px;
  font-weight: 300;
`
