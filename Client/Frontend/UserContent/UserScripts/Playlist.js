// MARK: - Media Detection

window.__firefox__.includeOnce("Playlist", function() {
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
    
    // Algorithm:
    // Generate a random number from 0 to 256
    // Roll-Over clamp to the range [0, 15]
    // If the index is 13, set it to 4.
    // If the index is 17, clamp it to [0, 3]
    // Subtract that number from 15 (XOR) and convert the result to hex.
    function uuid_v4() {
        // X >> 2 = X / 4 (integer division)
        
        // AND-ing (15 >> 0) roll-over clamps to 15
        // AND-ing (15 >> 2) roll-over clamps to 3
        // So '8' digit is clamped to 3 (inclusive) and all others clamped to 15 (inclusive).
        
        // 0 XOR 15 = 15
        // 1 XOR 15 = 14
        // 8 XOR 15 = 7
        // So N XOR 15 = 15 - N

        // UUID string format generated with array appending
        // Results in "10000000-1000-4000-8000-100000000000".replace(...)
        return ([1e7]+-1e3+-4e3+-8e3+-1e11).replace(/[018]/g, (X) => {
            return (X ^ (crypto.getRandomValues(new Uint8Array(1))[0] & (15 >> (X >> 2)))).toString(16);
        });
    }
    
    function $<tagNode>(node) {
        if (node) {
            if (!node.$<tagUUID>) {
                node.$<tagUUID> = uuid_v4();
            }
        }
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
                $<tagNode>(target);
                $<sendMessage>({
                    "securitytoken": "$<security_token>",
                    "name": name,
                    "src": target.src,
                    "pageSrc": window.location.href,
                    "pageTitle": document.title,
                    "mimeType": type,
                    "duration": clamp_duration(target.duration),
                    "detected": false,
                    "tagId": target.$<tagUUID>
                });
            }
            else {
                target.querySelectorAll('source').forEach(function(node) {
                    if (node.src && node.src !== "") {
                        if (node.closest('video') === target) {
                            $<tagNode>(target);
                            $<sendMessage>({
                                "securitytoken": "$<security_token>",
                                "name": name,
                                "src": node.src,
                                "pageSrc": window.location.href,
                                "pageTitle": document.title,
                                "mimeType": type,
                                "duration": clamp_duration(target.duration),
                                "detected": false,
                                "tagId": target.$<tagUUID>
                            });
                        }
                        
                        if (node.closest('audio') === target) {
                            $<tagNode>(target);
                            $<sendMessage>({
                                "securitytoken": "$<security_token>",
                                "name": name,
                                "src": node.src,
                                "pageSrc": window.location.href,
                                "pageTitle": document.title,
                                "mimeType": type,
                                "duration": clamp_duration(target.duration),
                                "detected": false,
                                "tagId": target.$<tagUUID>
                            });
                        }
                    }
                });
            }
        }
    }
    
    function $<setupLongPress>() {
        Object.defineProperty(window.__firefox__, '$<onLongPressActivated>', {
            enumerable: false,
            configurable: true,
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
                        $<tagNode>(targetVideo);
                        $<notify>(targetVideo, 'video');
                    }

                    if (targetAudio) {
                        $<tagNode>(targetAudio);
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
                $<tagNode>(node);
                $<sendMessage>({
                    "securitytoken": "$<security_token>",
                    "name": name,
                    "src": src,
                    "pageSrc": window.location.href,
                    "pageTitle": document.title,
                    "mimeType": mimeType,
                    "duration": clamp_duration(node.duration),
                    "detected": true,
                    "tagId": node.$<tagUUID>
                });
            } else {
                var target = node;
                document.querySelectorAll('source').forEach(function(node) {
                    if (node.src !== "") {
                        if (node.closest('video') === target) {
                            $<tagNode>(target);
                            $<sendMessage>({
                                "securitytoken": "$<security_token>",
                                "name": name,
                                "src": node.src,
                                "pageSrc": window.location.href,
                                "pageTitle": document.title,
                                "mimeType": mimeType,
                                "duration": clamp_duration(target.duration),
                                "detected": true,
                                "tagId": target.$<tagUUID>
                            });
                        }
                        
                        if (node.closest('audio') === target) {
                            $<tagNode>(target);
                            $<sendMessage>({
                                "securitytoken": "$<security_token>",
                                "name": name,
                                "src": node.src,
                                "pageSrc": window.location.href,
                                "pageTitle": document.title,
                                "mimeType": mimeType,
                                "duration": clamp_duration(target.duration),
                                "detected": true,
                                "tagId": target.$<tagUUID>
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
        
        function $<requestWhenIdleShim>(fn) {
          var start = Date.now()
          return setTimeout(function () {
            fn({
              didTimeout: false,
              timeRemaining: function () {
                return Math.max(0, 50 - (Date.now() - start))
              },
            })
          }, 2000);  // Resolution of 1000ms is fine for us.
        }

        function $<onReady>(fn) {
            if (document.readyState === "complete" || document.readyState === "ready") {
                setTimeout(fn, 1);
            } else {
                document.addEventListener("DOMContentLoaded", fn);
            }
        }
        
        function $<observePage>() {
            Object.defineProperty(HTMLMediaElement.prototype, '$<tagUUID>', {
                enumerable: false,
                configurable: false,
                writable: true,
                value: null
            });
            
            Object.defineProperty(HTMLMediaElement.prototype, 'src', {
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
            
            // When the page is idle
            // Fetch static video and audio elements
            var fetchExistingNodes = () => {
                $<requestWhenIdleShim>((deadline) => {
                    var videos = $<getAllVideoElements>();
                    var audios = $<getAllAudioElements>();
                    if (!videos) {
                        videos = [];
                    }
                    
                    if (!audios) {
                        audios = [];
                    }
                    
                    // Only on the next frame/vsync we notify the nodes
                    requestAnimationFrame(() => {
                        videos.forEach((e) => {
                            $<notifyNode>(e);
                        });
                    });
                    
                    // Only on the next frame/vsync we notify the nodes
                    requestAnimationFrame(() => {
                        audios.forEach((e) => {
                            $<notifyNode>(e);
                        });
                    });
                });
                
                // This function runs only once, so we remove as soon as the page is ready or complete
                document.removeEventListener("DOMContentLoaded", fetchExistingNodes);
            };
            
            // Listen for when the page is ready or complete
            $<onReady>(fetchExistingNodes);
        }

        $<observePage>();
    }
    
    function $<setupTagNode>() {
        Object.defineProperty(window.__firefox__, '$<mediaCurrentTimeFromTag>', {
            enumerable: false,
            configurable: true,
            value:
            function(tag) {
                for (element of document.querySelectorAll('video')) {
                    if (element.$<tagUUID> == tag) {
                        return clamp_duration(element.currentTime);
                    }
                }
                
                for (element of document.querySelectorAll('audio')) {
                    if (element.$<tagUUID> == tag) {
                        return clamp_duration(element.currentTime);
                    }
                }
                
                return 0.0;
            }
        });
        
        Object.defineProperty(window.__firefox__, '$<stopMediaPlayback>', {
            enumerable: false,
            configurable: true,
            value:
            function(tag) {
                for (element of document.querySelectorAll('video')) {
                    element.pause();
                }
                
                for (element of document.querySelectorAll('audio')) {
                    element.pause();
                }
                
                return 0.0;
            }
        });
    }
    
    // MARK: -----------------------------
    
    $<setupLongPress>();
    $<setupDetector>();
    $<setupTagNode>();
});
