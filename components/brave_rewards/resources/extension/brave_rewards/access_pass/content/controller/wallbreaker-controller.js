import runOnFirstVisible from '../../../../../../../common/pageLifecycle/run-on-first-visible'
import runOnLoaded from '../../../../../../../common/pageLifecycle/run-on-loaded'

function sendBackgroundMessage (message) {
  return new Promise (resolve => chrome.runtime.sendMessage(message, resolve))
}

export default class WallbreakerController {
  constructor({ fnDetectBypassablePaywall, fnBypassPaywall }) {
    this.bypassPaywall = fnBypassPaywall
    this.detectBypassablePaywall = fnDetectBypassablePaywall
    this.publisherHost = window.location.host
  }

  runForCurrentPage() {
    runOnLoaded(async () => {
      // TODO: first, communicate with braveRewards to check we have this feature on
      console.log('Wallbreaker: detecting if page has a bypassable paywall')
      const bypassObj = await this.detectBypassablePaywall()
      // todo: listen for paywall bypass requested, i.e. user says 'yes'

      console.log("Wallbreaker: >> Detect bypass paywall::", bypassObj)

      if (bypassObj.bypass) {
        // make sure we don't run paywall code for background tabs
        runOnFirstVisible(async () => {
          // Find out if user has exchanged BAT for paywall-bypass for this
          // publisher.
          const { canBypass } = await sendBackgroundMessage({
            type: 'get-paywall-can-bypass',
            publisherHost: this.publisherHost
          })
          console.log(`Wallbreaker: can bypass?`, canBypass)
          if (canBypass) {
            // Perform actual bypass
            console.log('Wallbreaker: bypassing paywall...')
            this.bypassPaywall()
          } else {
            // Wait until such a time that we can bypass (user may fill up credits
            // or click 'Unlock')
            const onMessage = (request, sender, sendResponse) => {
              if (request.type === 'bypass-status-changed' && request.canBypass) {
                chrome.runtime.onMessage.removeListener(onMessage)
                console.log('Wallbreaker: bypassing paywall...')
                this.bypassPaywall()
              }
            }
            chrome.runtime.onMessage.addListener(onMessage)
          }
        })
      }
    })
  }
}