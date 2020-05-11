// To opt in:
/*
  chrome.braveWallet.loadUI(() => {
    window.location.href = 'chrome://wallet'
  })
*/

chrome.braveWallet.shouldPromptForSetup((prompt) => {
  console.log('should prompt: ', prompt)
  // If we shouldn't prompt, then the extension is already loaded or
  // else the user has already explicitly opted in.
  if (!prompt) {
    chrome.braveWallet.loadUI(() => {
      window.location.href = 'chrome://wallet'
    })
  }
})
