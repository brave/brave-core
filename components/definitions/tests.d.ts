declare module 'chromedriver'

// Used in tests
declare namespace NodeJS {
  interface Global {
    requestAnimationFrame: any
    chrome: any
    window: any
    HTMLElement: any
    navigator: any
    document: any
  }
}
