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
  width: 370px;
  background: linear-gradient(-45deg, rgb(42, 31, 173) 0%, rgb(169, 27, 120) 100%) 0% 0% / 150% 150%;
  padding: 20px;
  display: flex;
  flex-direction: column;
  align-items: center;
  font-size: 16px;
  color: white;
  text-align: center;
`

const DialogTitleComponent = styled('h2')`
  font-size: 18px;
  font-weight: bold;
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
        <DialogTitleComponent>Want to keep reading?</DialogTitleComponent>
        <p>Get a temporary access pass to 5 articles on {publisherId}.</p>
        <p>No registration required.</p>
        <Button
          type="accent"
          brand="rewards"
          text="Tip 5 BAT"
          onClick={handleTipAction.bind(null, publisherId)}
          />
      </AppComponent>
    </ThemeProvider>
  )
}

const pageElement = document.querySelector('#root')

const publisherId = document.location.search.substr(1)

render(<App publisherId={publisherId} />, pageElement)
