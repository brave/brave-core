const config = { attributes: true, childList: true, subtree: true }
const textToMatch: Array<string> = ['Add to Chrome', 'Remove from Chrome']

const callback = (mutationsList: MutationRecord[], observer: MutationObserver) => {
  const buttons: NodeListOf<Element> = document.querySelectorAll('div.webstore-test-button-label')

  buttons.forEach((button: Element) => {
    const text: string = button.innerHTML
    if (textToMatch.includes(text)) {
      button.innerHTML = text.replace('Chrome', 'Brave')
    }
  })
}

const observer: MutationObserver = new MutationObserver(callback)

window.onload = () => {
  observer.observe(document, config)
}
