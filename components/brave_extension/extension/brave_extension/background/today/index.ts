const feedUrl = 'https://pcdn.brave.software/brave-today/feed.json'
import * as Background from '../../../../../common/Background'

// TODO: db
// TODO: BraveToday.Feed
let memoryFeedContents: any

let readLock: Promise<void> | null

function updateFeed() {
  readLock = new Promise(async function (resolve) {
    try {
      const feedResponse = await fetch(feedUrl)
      if (feedResponse.ok) {
        memoryFeedContents = await feedResponse.json()
        console.log('Got feed', memoryFeedContents)
        resolve()
      } else {
        console.error(`Not ok when fetching feed. Status ${feedResponse.status} (${feedResponse.statusText})`)
      }
    } catch (e) {
      console.error('Could not fetch feed contents from ', feedUrl)
      throw e
    } finally {

      readLock = null
    }
  })
}



// function on(messageType: string)
// TODO: make this a common thing and do explicit types for payloads like
// we do with redux action payloads.
Background.setListener<void>(
  Background.MessageType.indicatingOpen,
  async function (payload, sender, sendResponse) {
    console.log('indicatingOpen')
    updateFeed()
  }
)

Background.setListener<Background.GetFeedPayload>(
  Background.MessageType.getFeed,
  async function (req: Background.GetFeedPayload, sender, sendResponse) {
    console.log('asked to get feed')
    if (!memoryFeedContents) {
      // Fetch but only once at a time, and wait. No point returning no data
      if (!readLock) {
        updateFeed()
      }
      await readLock
    }
    // Only wait once. If there was an error or no data then return nothing.
    // TODO: return error status
    console.log('sending', memoryFeedContents)
    // @ts-ignore
    sendResponse(memoryFeedContents || {})
  }
)





// chrome.runtime.onMessageExternal.addListener(function (req, sender, sendResponse) {
//   console.log('got something', req, sender);
//   sendResponse({ got: true });
// })

// TODO: schedule to update feed
// updateFeed()
