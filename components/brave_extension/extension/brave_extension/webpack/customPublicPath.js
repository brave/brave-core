/* global __webpack_public_path__ __HOST__ __PORT__ */
/* eslint no-global-assign: 0 camelcase: 0 */

if (process.env.NODE_ENV === 'production') {
  __webpack_public_path__ = chrome.extension.getURL('/js/')
} else {
  // In development mode,
  const path = `//${__HOST__}:${__PORT__}/js/`
  if (window.location.protocol === 'https:' || window.location.search.indexOf('protocol=https') !== -1) {
    __webpack_public_path__ = `https:${path}`
  } else {
    __webpack_public_path__ = `http:${path}`
  }
}
