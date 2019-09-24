export default function sendBackgroundMessage (message) {
  return new Promise (resolve => chrome.runtime.sendMessage(message, resolve))
}
