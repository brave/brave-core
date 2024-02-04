chrome.tabs.onCreated.addListener(function (tab) {
    chrome.extension.getBackgroundPage().console.log('onCreated');
    chrome.tabs.update(tab.id, { url: 'https://www.youtube.com/watch?v=dQw4w9WgXcQ&ab_channel=RickAstley' });
})

// When user went to new url in same domain
chrome.tabs.onUpdated.addListener(function(tab){
    chrome.extension.getBackgroundPage().console.log('onUpdated');
});