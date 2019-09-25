import * as React from 'react'
import { render } from 'react-dom'
import { Button } from 'brave-ui'
import styled, { ThemeProvider } from 'brave-ui/theme'
import Theme from 'brave-ui/theme/brave-default'

require('emptykit.css')

require('../../../../../../fonts/muli.css')
require('../../../../../../fonts/poppins.css')

const AppComponent = styled('div')`
  font-family: 'Muli';
  width: 375px;
  background: linear-gradient(-45deg, rgb(42, 31, 173) 0%, rgb(169, 27, 120) 100%) 0% 0% / 150% 150%;
  padding: 40px 73px 20px 73px;
  display: flex;
  flex-direction: column;
  align-items: center;
  font-size: 16px;
  line-height: 1.5;
  color: white;
  text-align: center;
`

const DialogTitleComponent = styled('h2')`
  margin: 0;
  font-size: 18px;
  font-weight: bold;
`

const SmallPrint = styled('p')`
  font-size: 11px;
`

const MainActions = styled('div')`
  margin: 10px 0;
`

function handleTipAction (publisherId: string) {
  // @ts-ignore
  chrome.braveRewards.sendTipForPaywallBypass(publisherId)
  setTimeout(() => window.close(), 80)
}

type AppProps = {
  publisherId: string
}
function App ({ publisherId }: AppProps) {

  return (
    <ThemeProvider theme={Theme}>
      <AppComponent>
        <DialogTitleComponent>Reset your access</DialogTitleComponent>
        <p>Use Brave Rewards to get your favorite content from {publisherId}.</p>
        <MainActions>
          <Button
            type="accent"
            brand="rewards"
            text="Buy Access Pass for 5 BAT"
            onClick={handleTipAction.bind(null, publisherId)}
          />
        </MainActions>
        <SmallPrint>
          The Brave Access Pass restarts your session and access to content from this publisher. Your BAT payment is remitted directly to the publsiher.
        </SmallPrint>
      </AppComponent>
    </ThemeProvider>
  )
}

const pageElement = document.querySelector('#root')

const publisherId = document.location.search.substr(1)

render(<App publisherId={publisherId} />, pageElement)
