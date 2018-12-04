import renderTipDialog from '../user-tip/userTipDialog'

// Receive message from background script
chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  const { userAction, tweetId }: { userAction: string, tweetId: string } = request
  // Tell the background script we're here
  // so that it doesn't add this content script to the frame again.
  sendResponse({ received: true })
  if (userAction === 'USER_TIP') {
    if (!tweetId) {
      console.error('Received a USER_TIP but no tweetId')
      return
    }
    console.log(`Received a USER_TIP for tweet ${tweetId}`)
    detectShowUserTip(tweetId)
  }
})

function detectShowUserTip (tweetId: string) {
  console.log('onUserTip', tweetId)
  const tweetItems = document.querySelectorAll(`[data-retweet-id="${tweetId}"], [data-tweet-id="${tweetId}"]`)
  // TODO: work out which one was clicked
  if (tweetItems.length) {
    const currentActionElement = tweetItems[0].querySelector('.js-actionFavorite')
    if (currentActionElement) {
      const stableActionElement = currentActionElement.parentElement
      if (stableActionElement) {
        window.requestAnimationFrame(() => {
          offerUserTip(stableActionElement)
        })
        return
      }
    }
  }
  // const anchorElement = document.querySelector(`[aria-describedby="profile-tweet-action-favorite-count-aria-${tweetId}"]`)
  console.log('onUserTip: could not get anchor element', tweetItems)
}

let dialogEl: HTMLElement | null

async function offerUserTip (anchorElement: Element) {
  console.log('offerUserTip', anchorElement)
  removeDialog()
  dialogEl = document.createElement('div')
  const anchorElementRect = anchorElement.getBoundingClientRect()
  // TODO: Show bottom / top and left / right
  // according to viewport space available.
  const dialogPosition = {
    x: anchorElementRect.left + window.scrollX,
    y: anchorElementRect.bottom + window.scrollY + 10
  }
  dialogEl.style.position = 'absolute'
  dialogEl.style.left = `${dialogPosition.x}px`
  dialogEl.style.top = `${dialogPosition.y}px`
  renderTipDialog(dialogEl, 'topLeft')
  document.body.appendChild(dialogEl)
  document.body.addEventListener('click', onDocumentDialogBackgroundClick)
}

function onDocumentDialogBackgroundClick () {
  console.log('onDocumentDialogBackgroundClick')
  document.body.removeEventListener('click', onDocumentDialogBackgroundClick)
  removeDialog()
}

function removeDialog () {
  if (dialogEl) {
    dialogEl.remove()
    dialogEl = null
  }
}
// get heart button: '[aria-describedby="profile-tweet-action-favorite-count-aria-853292140620730369"]'
