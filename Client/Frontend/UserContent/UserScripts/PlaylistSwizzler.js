
// Stub out the MediaSource API so video players do not attempt to use `blob` for streaming
if (window.MediaSource || window.WebKitMediaSource || window.HTMLMediaElement && HTMLMediaElement.prototype.webkitSourceAddId) {
    
    window.MediaSource = null;
    window.WebKitMediaSource = null;
    //HTMLMediaElement.prototype.webkitSourceAddId = null;
    //window.SourceBuffer = null;
    
    delete window.MediaSource;
    delete window.WebKitMediaSource;
}
