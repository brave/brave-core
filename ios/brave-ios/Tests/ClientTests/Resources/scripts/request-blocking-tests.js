(() => {
  "use strict"
  /**
   * Send the results of farbled APIs to iOS so it can see if its properly farbled
   * @param {boolean} isBlocked A boolean indicating if the request was blocked
   * @returns A Promise that resolves new hide selectors
   */
  const sendTestResults = (blockedFetch, blockedXHR) => {
    return webkit.messageHandlers['SendTestRequestResult'].postMessage({
      'blockedFetch': blockedFetch,
      'blockedXHR': blockedXHR
    })
  }

  const testXHR = () => {
    return new Promise((resolve, _) => {
      const req = new XMLHttpRequest()
      req.addEventListener("load", () => {
        resolve(false)
      })
      req.addEventListener("error", () => {
        resolve(true)
      })
      req.open("GET", "http://example.com/movies.json", true)
      req.send()
    })
  }
  
  const testFetch = async () => {
    let blockedFetch = false
    let blockedXHR = false
    
    try {
      const response = await fetch("http://example.com/movies.json")
      return false
    } catch (error) {
      return true
    }
  }

  const testAPIs = async () => {
    let blockedFetch = await testFetch()
    let blockedXHR = await testXHR()

    sendTestResults(blockedFetch, blockedXHR)
  }

  testAPIs()
})()
