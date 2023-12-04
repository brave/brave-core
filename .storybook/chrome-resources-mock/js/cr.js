/**
 * 
 * @param {string} eventName 
 * @param {Function} handler 
 */
export function addWebUIListener(eventName, handler) {}

/**
 * 
 * @param {string} message 
 * @param  {...any} args 
 */
export function sendWithPromise(message, ...args) {
  return new Promise(() => console.log('sendWithPromise', message, args))
}
