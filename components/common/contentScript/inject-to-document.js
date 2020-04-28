export default function injectToDocument (isolatedFn, ...codeVars) {
  // Convert function to single-scope and stringify
  const codeToEval = `(
    ${isolatedFn.toString()}
  )(${codeVars.map(prop => JSON.stringify(prop)).join(', ')})`

  function inject () {
    const scriptEl = document.createElement('script')
    scriptEl.async = true
    scriptEl.textContent = codeToEval
    ;(document.body || document.documentElement).appendChild(scriptEl)
    requestAnimationFrame(() => {
      scriptEl.remove()
    })
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', inject)
  } else {
    inject()
  }
}
