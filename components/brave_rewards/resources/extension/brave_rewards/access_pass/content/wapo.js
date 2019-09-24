import WallbreakerController from './controller/wallbreaker-controller'

// true if cookies indicating that paywall is on are set
function detectBypassablePaywall() {
  console.log('Wallbreaker [wapo]: detecting if page has a bypassable paywall')

  let rplmct = getCookie("rplmct");
  let rplm2 = getCookie("rplm2");
  let where = document.location.href;
  
  // skip landing page
  if (where == "https://www.washingtonpost.com/") {
  	return Promise.resolve({where: "", bypass: false, rplmct: rplmct, rplm2: rplm2})	
  }
  // is bypassable if `rplmct` is == 3
  if (rplmct == "3") {
	  return Promise.resolve({where: where, bypass: true, rplmct: rplmct, rplm2: rplm2})
  }

  return Promise.resolve({where: where, bypass: false, rplmct: rplmct, rplm2: rplm2})
}

function bypassPaywall() {
  console.log('Wallbreaker [barrons]: bypassing paywall...')
  chrome.runtime.sendMessage({
    type: 'perform-bypass',
    bypassArgs: {
      cookiesToDelete: [ 'rplm2', 'rplmct' ],
      cookiesToExtend: [],
      domain: ".washingtonpost.com"
    }
  })
}

new WallbreakerController({
  fnDetectBypassablePaywall: detectBypassablePaywall,
  fnBypassPaywall: bypassPaywall
}).runForCurrentPage()

function getCookie(name) {
  var value = "; " + document.cookie;
  var parts = value.split("; " + name + "=");
  if (parts.length == 2) return parts.pop().split(";").shift();
}