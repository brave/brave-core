cr.define('brave_rewards.donate', function () {

  function closeDialog() {
    // Note: dialogClose supports an object to send back to dialog class
    chrome.send('dialogClose')
  }

  return {
    initialize: () => {
      const dialogArgsRaw = chrome.getVariableValue('dialogArguments')
      try {
        const args = JSON.parse(dialogArgsRaw)
        console.log('incoming dialog args', args)
      }
      catch (e) {
        console.error('Error parsing incoming dialog args', dialogArgsRaw, e)
      }
      document.querySelector('.dialog--close').addEventListener('click', () => {
        closeDialog()
      })
    }
  }
})
document.addEventListener('DOMContentLoaded', brave_rewards.donate.initialize)
