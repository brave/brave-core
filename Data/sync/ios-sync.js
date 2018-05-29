console.log = (function (old_function) {
    return function (text) {
        old_function(text);
        document.write("<div style='font-size:25'>"+ text +"</div>");
    };
} (console.log.bind(console)));

var callbackList = {} // message name to callback function

if (!self.chrome || !self.chrome.ipc) {
    self.chrome = {}
    const ipc = {}
    const stringifyArrayCleaner = (k,v) => {
        if (v instanceof Uint8Array) {
            return Array.from(v)
        }
        return v
    }

    ipc.once = (message, cb) => {
        callbackList[message] = cb
        window.webkit.messageHandlers.syncToIOS_on.postMessage(JSON.stringify({message: message}, stringifyArrayCleaner));
    }
    
    ipc.on = ipc.once

    ipc.send = (message, arg1, arg2, arg3, arg4) => {
        window.webkit.messageHandlers.syncToIOS_send.postMessage(JSON.stringify({message: message, arg1: arg1, arg2: arg2, arg3: arg3, arg4: arg4}, stringifyArrayCleaner));
    }

    self.chrome.ipc = ipc
    chrome.ipcRenderer = chrome.ipc
}
