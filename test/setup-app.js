import jsdom from 'jsdom'
const {JSDOM} = jsdom
const {document} = (new JSDOM('<!doctype html><html><body></body></html>')).window
global.document = document
global.window = document.defaultView
global.navigator = global.window.navigator
global.HTMLElement = global.window.HTMLElement

if (global.chrome === undefined) {
  global.chrome = {
    runtime: {
      onMessage: {
        addListener: function () {
        }
      },
      onConnect: {
        addListener: function () {
        }
      }
    },
    tabs: {
      queryAsync: function () {
        return Promise.resolve([{
          url: 'https://www.brave.com'
        }])
      }
    },
    braveShields: {
      onBlocked: {
        addListener: function () {
        }
      }
    },
    contentSettings: {
      braveAdBlock: {
        setAsync: function () {
          return Promise.resolve()
        },
        getAsync: function () {
          return Promise.resolve({
            setting: 'block'
          })
        }
      },
      braveTrackingProtection: {
        setAsync: function () {
          return Promise.resolve()
        },
        getAsync: function () {
          return Promise.resolve({
            setting: 'block'
          })
        }
      }
    }
  }
}
