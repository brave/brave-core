import WallbreakerController from './controller/wallbreaker-controller'
import PerformBypassInBackgroundScript from './controller/perform-bypass-in-background'

function detectBypassablePaywall() {
  console.log('Wallbreaker [barons]: detecting if page has a bypassable paywall')
  return Promise.resolve(true)
}

function bypassPaywall() {
  console.log('Wallbreaker [barrons]: bypassing paywall in background script...')
  return PerformBypassInBackgroundScript({
    cookiesToDelete: [ 'cookie1, cookie2' ],
    cookiesToExtend: [ 'cookie3' ]
  })
}

new WallbreakerController({
  fnBypassPaywall: detectBypassablePaywall,
  fnBypassPaywall: bypassPaywall
}).runForCurrentPage()