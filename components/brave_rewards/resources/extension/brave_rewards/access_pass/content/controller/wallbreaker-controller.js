import runOnFirstVisible from '../../../../../../../common/pageLifecycle/run-on-first-visible'
import runOnLoaded from '../../../../../../../common/pageLifecycle/run-on-loaded'
import sendBackgroundMessage from './send-background-message'

export default class WallbreakerController {
  constructor({ fnDetectBypassablePaywall, fnBypassPaywall }) {
    this.fnBypassPaywall = fnBypassPaywall
    this.fnDetectBypassablePaywall = fnDetectBypassablePaywall
    this.publisherHost = window.location.host
  }

  detectBypassablePaywall() {
    // Detect site-specific paywall
    return this.fnDetectBypassablePaywall()
  }

  async bypassPaywall() {
    // Perform site-specific bypass
    const bypass = await this.fnBypassPaywall()
    // Return status to background so can
    // report on bypass or reload this and relevant tabs
    if (bypass.didBypass) {
      sendBackgroundMessage({
        type: 'did-bypass-paywall',
        publisherHost: this.publisherId,
        urlToShow: bypass.urlReload ? bypass.urlReload : this.currentPageUrl
      })
    }
  }

  runForCurrentPage() {
    this.currentPageUrl = window.location.href
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