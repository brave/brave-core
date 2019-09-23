import WallbreakerController from './controller/wallbreaker-controller'

function detectBypassablePaywall() {
  console.log('Wallbreaker [barons]: detecting if page has a bypassable paywall')
  return Promise.resolve(true)
}

function bypassPaywall() {
  console.log('Wallbreaker [barrons]: bypassing paywall...')
}

new WallbreakerController({
  fnBypassPaywall: detectBypassablePaywall,
  fnBypassPaywall: bypassPaywall
}).runForCurrentPage()