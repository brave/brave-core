const redirectAmp = () => {
  // Check whether it's AMP html
  let h = document.getElementsByTagName('html')
  if (h[0].getAttribute('amp') !== null
    || h[0].getAttribute('⚡') !== null
    || document.location.href.substr(0, 29).toLowerCase() === 'https://www.google.com/amp/s/') {

    // Try and find a canonical link
    let canonicals = document.querySelectorAll('link[rel=canonical]')
    for (let i = 0; i < canonicals.length; i++) {
      let canonicalURL = canonicals[i].getAttribute('href')
      if (canonicalURL !== null && canonicalURL !== window.location.href.split('#')[0]) {
        window.location.replace(canonicalURL)
      }
    }
  }
}

document.addEventListener('DOMContentLoaded', redirectAmp, false)
