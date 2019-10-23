import styled from 'brave-ui/theme'
import oneTimeBgUrl from './assets/one-time-bg.svg'
import monthlyBgUrl from './assets/monthly-bg.svg'
import tweetUrl from './assets/tweet.svg'

interface StyleProps {
  isMonthly?: boolean
}

export const StyledWrapper = styled<StyleProps, 'div'>('div')`
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

export const StyledTitle = styled<{}, 'span'>('span')`
  display: block;
  font-size: 16px;
  font-weight: 600;
  letter-spacing: 0px;
  margin-top: 25px;
`

export const StyledSubTitle = styled<{}, 'span'>('span')`
  display: block;
  font-size: 12px;
  font-weight: 400;
  letter-spacing: 0px;
  margin-top: 10px;
`

export const StyledTweet = styled<{}, 'div'>('div')`
  cursor: pointer;
  width: 96px;
  height: 96px;
  margin: 25px auto 0 auto;
  background: url(${tweetUrl});
`
