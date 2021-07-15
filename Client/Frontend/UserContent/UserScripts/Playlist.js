// The below is needed because the script may not be web-packed into a bundle so it may be missing the run-once code

// MARK: - Include Once

if (!window.__firefox__) {
    window.__firefox__ = {};
}

if (!window.__firefox__.includeOnce) {
    window.__firefox__ = {};
    window.__firefox__.includeOnce = function(key, func) {
        var keys = {};
        if (!keys[key]) {
            keys[key] = true;
            func();
        }
    };
}

// MARK: - Media Detection

window.__firefox__.includeOnce("$<Playlist>", function() {
    function is_nan(value) {
        return typeof value === "number" && value !== value;
    }
    
    function is_infinite(value) {
        return typeof value === "number" && (value === Infinity || value === -Infinity);
    }
    
    function clamp_duration(value) {
        if (is_nan(value)) {
            return 0.0;
        }
        
        if (is_infinite(value)) {
            return Number.MAX_VALUE;
        }
        return value;
    }
    
    function $<sendMessage>(message) {
        if (window.webkit.messageHandlers.$<handler>) {
            window.webkit.messageHandlers.$<handler>.postMessage(message);
        }
    }
    
    function $<notify>(target, type) {
        if (target) {
            var name = target.title;
            if (name == null || typeof name == 'undefined' || name == "") {
                name = document.title;
            }
            
            if (target.src && target.src !== "") {
                $<sendMessage>({
                    "securitytoken": "$<security_token>",
                    "name": name,
                    "src": target.src,
                    "pageSrc": window.location.href,
                    "pageTitle": document.title,
                    "mimeType": type,
                    "duration": clamp_duration(target.duration),
                    "detected": false,
                });
            }
            else {
                target.querySelectorAll('source').forEach(function(node) {
                    if (node.src && node.src !== "") {
                        if (node.closest('video') === target) {
                            $<sendMessage>({
                                "securitytoken": "$<security_token>",
                                "name": name,
                                "src": node.src,
                                "pageSrc": window.location.href,
                                "pageTitle": document.title,
                                "mimeType": type,
                                "duration": clamp_duration(target.duration),
                                "detected": false,
                            });
                        }
                        
                        if (node.closest('audio') === target) {
                            $<sendMessage>({
                                "securitytoken": "$<security_token>",
                                "name": name,
                                "src": node.src,
                                "pageSrc": window.location.href,
                                "pageTitle": document.title,
                                "mimeType": type,
                                "duration": clamp_duration(target.duration),
                                "detected": false,
                            });
                        }
                    }
                });
            }
        }
    }
    
    function $<setupLongPress>() {
        Object.defineProperty(window, '$<onLongPressActivated>', {
          value:
            function(localX, localY) {
                function execute(page, offsetX, offsetY) {
                    var target = page.document.elementFromPoint(localX - offsetX, localY - offsetY);
                    var targetVideo = target ? target.closest("video") : null;
                    var targetAudio = target ? target.closest("audio") : null;

                    // Video or Audio might have some sort of overlay..
                    // Like player controls for pause/play, etc..
                    // So we search for video/audio elements relative to touch position.
                    if (!targetVideo && !targetAudio) {
                        var touchX = localX + (page.scrollX + offsetX);
                        var touchY = localY + (page.scrollY + offsetY);
                    
                        var videoElements = page.document.querySelectorAll('video');
                        for (element of videoElements) {
                            var rect = element.getBoundingClientRect();
                            var x = rect.left + (page.scrollX + offsetX);
                            var y = rect.top + (page.scrollY + offsetY);
                            var w = rect.right - rect.left;
                            var h = rect.bottom - rect.top;
                            
                            if (touchX >= x && touchX <= (x + w) && touchY >= y && touchY <= (y + h)) {
                                targetVideo = element;
                                break;
                            }
                        }
                        
                        var audioElements = page.document.querySelectorAll('audio');
                        for (element of audioElements) {
                            var rect = element.getBoundingClientRect();
                            var x = rect.left + (page.scrollX + offsetX);
                            var y = rect.top + (page.scrollY + offsetY);
                            var w = rect.right - rect.left;
                            var h = rect.bottom - rect.top;
                            
                            if (touchX >= x && touchX <= (x + w) && touchY >= y && touchY <= (y + h)) {
                                targetAudio = element;
                                break;
                            }
                        }
                        
                        // No elements found nearby so do nothing..
                        if (!targetVideo && !targetAudio) {
                            // webkit.messageHandlers.$<handler>.postMessage({});
                            return;
                        }
                    }
                    
                    // Elements found
                    if (targetVideo) {
                        $<notify>(targetVideo, 'video');
                    }

                    if (targetAudio) {
                        $<notify>(targetAudio, 'audio');
                    }
                }
                
                // Any videos in the current `window.document`
                // will have an offset of (0, 0) relative to the window.
                execute(window, 0, 0);
                
                // Any videos in a `iframe.contentWindow.document`
                // will have an offset of (0, 0) relative to its contentWindow.
                // However, it will have an offset of (X, Y) relative to the current window.
                for (frame of document.querySelectorAll('iframe')) {
                    // Get the frame's bounds relative to the current window.
                    var bounds = frame.getBoundingClientRect();
                    execute(frame.contentWindow, bounds.left, bounds.top);
                }
            }
        });
    }
    
    // MARK: ---------------------------------------
    
    function $<setupDetector>() {
        function $<notifyNodeSource>(node, src, mimeType) {
            var name = node.title;
            if (name == null || typeof name == 'undefined' || name == "") {
                name = document.title;
            }

            if (mimeType == null || typeof mimeType == 'undefined' || mimeType == "") {
                if (node.constructor.name == 'HTMLVideoElement') {
                    mimeType = 'video';
                }

                if (node.constructor.name == 'HTMLAudioElement') {
                    mimeType = 'audio';
                }
                
                if (node.constructor.name == 'HTMLSourceElement') {
                    videoNode = node.closest('video');
                    if (videoNode != null && typeof videoNode != 'undefined') {
                        mimeType = 'video'
                    } else {
                        mimeType = 'audio'
                    }
                }
            }

            if (src !== "") {
                $<sendMessage>({
                    "securitytoken": "$<security_token>",
                    "name": name,
                    "src": src,
                    "pageSrc": window.location.href,
                    "pageTitle": document.title,
                    "mimeType": mimeType,
                    "duration": clamp_duration(node.duration),
                    "detected": true
                });
            } else {
                var target = node;
                document.querySelectorAll('source').forEach(function(node) {
                    if (node.src !== "") {
                        if (node.closest('video') === target) {
                            $<sendMessage>({
                                "securitytoken": "$<security_token>",
                                "name": name,
                                "src": node.src,
                                "pageSrc": window.location.href,
                                "pageTitle": document.title,
                                "mimeType": mimeType,
                                "duration": clamp_duration(target.duration),
                    "detected": true
                            });
                        }
                        
                        if (node.closest('audio') === target) {
                            $<sendMessage>({
                                "securitytoken": "$<security_token>",
                                "name": name,
                                "src": node.src,
                                "pageSrc": window.location.href,
                                "pageTitle": document.title,
                                "mimeType": mimeType,
                                "duration": clamp_duration(target.duration),
                    "detected": true
                            });
                        }
                    }
                });
            }
        }

        function $<notifyNode>(node) {
            $<notifyNodeSource>(node, node.src, node.type);
        }

        function $<getAllVideoElements>() {
            return document.querySelectorAll('video');
        }

        function $<getAllAudioElements>() {
            return document.querySelectorAll('audio');
        }

        function $<onReady>(fn) {
            if (document.readyState === "complete" || document.readyState === "interactive") {
                setTimeout(fn, 1);
            } else {
                document.addEventListener("DOMContentLoaded", fn);
            }
        }
        
        function $<observePage>() {
            Object.defineProperty(HTMLVideoElement.prototype, 'src', {
                enumerable: true,
                configurable: false,
                get: function(){
                    return this.getAttribute('src')
                },
                set: function(value) {
                    // Typically we'd call the original setter.
                    // But since the property represents an attribute, this is okay.
                    this.setAttribute('src', value);
                    //$<notifyNode>(this); // Handled by `setVideoAttribute`
                }
            });
            
            Object.defineProperty(HTMLAudioElement.prototype, 'src', {
                enumerable: true,
                configurable: false,
                get: function(){
                    return this.getAttribute('src')
                },
                set: function(value) {
                    // Typically we'd call the original setter.
                    // But since the property represents an attribute, this is okay.
                    this.setAttribute('src', value);
                    //$<notifyNode>(this); // Handled by `setAudioAttribute`
                }
            });
            
            var setVideoAttribute = HTMLVideoElement.prototype.setAttribute;
            HTMLVideoElement.prototype.setAttribute = function(key, value) {
                setVideoAttribute.call(this, key, value);
                if (key.toLowerCase() == 'src') {
                    $<notifyNode>(this);
                }
            }
            
            var setAudioAttribute = HTMLAudioElement.prototype.setAttribute;
            HTMLAudioElement.prototype.setAttribute = function(key, value) {
                setAudioAttribute.call(this, key, value);
                if (key.toLowerCase() == 'src') {
                    $<notifyNode>(this);
                }
            }
        }

        $<observePage>();
    }
    
    
    // MARK: -----------------------------
    
    $<setupLongPress>();
    $<setupDetector>();
});
