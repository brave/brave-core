// Demo in-memory store of which publishers (sites) have how many
// unlocked articles remaining.
const demoPaywallCountStore = new Map();

// TODO: If we want to perform wallbreaking for some sites just from background
// script, then intercept page or cookie load
// and call `getCanBypassForPublisher`.


function getCanBypassForPublisher (tabId, publisherId) {
  const count = demoPaywallCountStore.get(publisherId)
  if (count) {
    // reduce for demo
    demoPaywallCountStore.set(publisherId, count - 1)
    return true
  }
  chrome.braveRewards.offerPaywallBypass(tabId, publisherId)
  return false
}

chrome.runtime.onMessage.addListener(
  function (request, sender, sendResponse) {
    const tabId = sender.tab.id
    if (request.type === 'get-paywall-can-bypass') {
      const publisherId = request.publisherHost
      const canBypass = getCanBypassForPublisher(tabId, publisherId)
      // Let page know result for this time, though
      // if it's false, it can change to true in the future after
      // user unlock.
      sendResponse({ canBypass })
    }
  }
)

chrome.braveRewards.onPaywallBypassRequested.addListener(
  function (publisherId) {
    demoPaywallCountStore.set(publisherId, 5)
    notifyBypassStatusEnabledForPublisher(publisherId)
  }
)

function notifyBypassStatusEnabledForPublisher (publisherId) {
  // TODO: send message to all tab content scripts which have
  // this publisher Id
  chrome.tabs.query({active: true, currentWindow: true}, function(tabs) {
    chrome.tabs.sendMessage(tabs[0].id, {
      type: 'bypass-status-changed',
      canBypass: true
    });
  });
}