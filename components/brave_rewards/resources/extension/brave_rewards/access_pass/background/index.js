// Demo in-memory store of which publishers (sites) have how many
// unlocked articles remaining.
// const demoPaywallCountStore = new Map();

function getCanBypassForPublisher (tabId, publisherId) {
  // const count = demoPaywallCountStore.get(publisherId)
  // if (count) {
  //   // reduce for demo
  //   demoPaywallCountStore.set(publisherId, count - 1)
  //   return true
  // }
  // For now, we only allow looking at existing publisher paywall
  // strategy.
  // Therefore, we never unblock a new paywall until the user
  // has seen the popup, even if they have seen the same paywall before.
  // In other words, if the paywall re-appears, do not bypass it
  // until the user has sent the tip to bypass.
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
    } else if (request.type === 'perform-bypass') {
      sendResponse(performUnblockForSite(tabId, request.bypassArgs))
    } else if (request.type === 'did-bypass-paywall') {
      const publisherId = request.publisherHost
      const url = request.urlToShow
      performPostUnblockActions(tabId, publisherId, url)
    }
  }
)

chrome.braveRewards.onPaywallBypassRequested.addListener(
  function (publisherId) {
    // demoPaywallCountStore.set(publisherId, 5)
    notifyBypassStatusEnabledForPublisher(publisherId)
  }
)

function notifyBypassStatusEnabledForPublisher (publisherId) {
  // TODO: consider sending message to all tab content scripts which have
  // this publisher Id. However, we may not want to do this as could
  // quickly get past publisher's free-article count.
  chrome.tabs.query({active: true, currentWindow: true}, function(tabs) {
    chrome.tabs.sendMessage(tabs[0].id, {
      type: 'bypass-status-changed',
      canBypass: true
    });
  });
}

const getAllCookies = (arg) => new Promise(resolve => chrome.cookies.getAll(arg, resolve))
const removeCookie = (arg) => new Promise(resolve => chrome.cookies.remove(arg, resolve))

async function performUnblockForSite (tabId, bypassArgs) {
  const {
    cookiesToDelete,
    cookiesToExtend,
    domain,
    url
  } = bypassArgs
  let success = false
  const deleteOps = []
  if (cookiesToDelete) {
    const cookies = await getAllCookies({ domain })
    for (var i = cookies.length - 1; i >= 0; i--) {
      if (cookiesToDelete.includes(cookies[i].name)) {
        let name = cookies[i].name;
        deleteOps.push(removeCookie({ url, name }))
        // Guess that we succeeded if we got any cookie match.
        success = true
      }
    }
  }
  if (cookiesToExtend) {}
  await Promise.all(deleteOps)
  return success
}

function performPostUnblockActions (activePublisherTabId, publisherId, url) {
  // TODO: consider sending message to all tab content scripts which have
  // this publisher Id. However, we may not want to do this as could
  // quickly get past publisher's free-article count.
  // TODO: if paywall was removed via DOM then we may not need to refresh,
  // but all paywalls are currently using cookie technique.
  console.log('performPostUnblockActions', activePublisherTabId, publisherId)
  chrome.tabs.update(activePublisherTabId, { url })
}