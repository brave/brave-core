import generateRandomId from '../../../../../../common/generate-random-id'

let hasInit = false
let currentPageId

function setNewPage() {
  currentPageId = generateRandomId()
}

function init() {
  hasInit = true
  setNewPage()
  // TODO: regenerate pageId on pushstate / popstate / history.back / etc
  // to handle ajax navigated sites
}

export default function filterSlotsToOneBestSlot (batAds) {
  if (!hasInit) {
    init()
  }
  const articleTitles = document.querySelectorAll('h1')

  if (!articleTitles.length) {
    console.log('BATSense (one-best-onpage): no articleTitles')
    return [ batAds[0] ]
  }
  // TODO: try to detect if first h1 is logo (bad practice for SEO,
  // but some do it).
  // For now, choose the first.
  let targetElement = articleTitles[0]
  let targetElementRect = targetElement.getBoundingClientRect()
  if (articleTitles.length > 1) {
    // choose last one that's still inside viewport
    for (let i = 1; i < articleTitles.length; i++) {
      const rect = articleTitles[i].getBoundingClientRect()
      if (rect.y < window.innerWidth && rect.y > targetElementRect.y) {
        targetElement = articleTitles[i]
        targetElementRect = rect
      }
    }
  }
  const targetPos = [
    targetElementRect.x + (targetElementRect.width / 2),
    targetElementRect.y + targetElementRect.height
  ]
  let closestSlot
  let closestDistance
  for (const batAd of batAds) {
    const pos = guessValidPosForElement(batAd.element)
    const distance = Math.hypot(
      pos[0] - targetPos[0],
      pos[1] - targetPos[1]
    )
    console.log('distance compare', { pos, targetPos, distance, batAd, closestSlot, closestDistance })
    if (!closestSlot || (distance > 0 && distance < closestDistance)) {
      console.log('setting closest')
      closestSlot = batAd
      closestDistance = distance
    }
  }
  return [ closestSlot ]
}

function guessValidPosForElement(element) {
  let { x, y } = element.getBoundingClientRect()
  while (!x && !y) {
    element = element.parentElement
    if (!element) {
      break
    }
    const rect = element.getBoundingClientRect()
    x = rect.x
    y = rect.y
  }
  return [x, y]
}