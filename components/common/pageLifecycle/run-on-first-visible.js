export default function runOnFirstVisible(fn) {
  if (document.visibilityState === 'visible') {
    fn()
  } else {
    function onVisible() {
      if (document.visibilityState === 'visible') {
        document.removeEventListener('visibilitychange', onVisible)
        fn()
      }
    }
    document.addEventListener('visibilitychange', onVisible)
  }
}
