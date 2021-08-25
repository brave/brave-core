// MARK: - Media Detection

window.__firefox__.includeOnce("PlaylistDetector", function() {
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
            
            if (target.src !== "") {
                $<sendMessage>({
                    "securitytoken": "$<security_token>",
                    "name": name,
                    "src": target.src,
                    "pageSrc": window.location.href,
                    "pageTitle": document.title,
                    "mimeType": type,
                    "duration": clamp_duration(target.duration),
                    "detected": true,
                });
            }
            else {
                document.querySelectorAll('source').forEach(function(node) {
                    if (node.src !== "") {
                        if (node.closest('video') === target) {
                            $<sendMessage>({
                                "securitytoken": "$<security_token>",
                                "name": name,
                                "src": node.src,
                                "pageSrc": window.location.href,
                                "pageTitle": document.title,
                                "mimeType": type,
                                "duration": clamp_duration(target.duration),
                                "detected": true,
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
                                "detected": true,
                            });
                        }
                    }
                });
            }
        }
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

        function $<observeNode>(node) {
            if (node.observer == null || node.observer === undefined) {
                node.observer = new MutationObserver(function (mutations) {
                    $<notifyNode>(node);
                });
                node.observer.observe(node, { attributes: true, attributeFilter: ["src"] });
                $<notifyNode>(node);

                node.addEventListener('loadedmetadata', function() {
                    $<notifyNode>(node);
                });
            }
        }

        function $<observeDocument>(node) {
            if (node.observer == null || node.observer === undefined) {
                node.observer = new MutationObserver(function (mutations) {
                    mutations.forEach(function (mutation) {
                        mutation.addedNodes.forEach(function (node) {
                            if (node.constructor.name == "HTMLVideoElement") {
                                $<observeNode>(node);
                            }
                            else if (node.constructor.name == "HTMLAudioElement") {
                                $<observeNode>(node);
                            }
                        });
                    });
                });
                node.observer.observe(node, { subtree: true, childList: true });
            }
        }

        function $<observeDynamicElements>(node) {
            var original = node.createElement;
            node.createElement = function (tag) {
                if (tag === 'audio' || tag === 'video') {
                    var result = original.call(node, tag);
                    $<observeNode>(result);
                    $<notifyNode>(result);
                    return result;
                }
                return original.call(node, tag);
            };
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
            $<observeDocument>(document);
            $<observeDynamicElements>(document);

            $<onReady>(function() {
                $<getAllVideoElements>().forEach(function(node) {
                    $<observeNode>(node);
                });

                $<getAllAudioElements>().forEach(function(node) {
                    $<observeNode>(node);
                });
                
                $<sendMessage>({"pageLoad": true})
            });
            
            // Timeinterval is needed for DailyMotion as their DOM is bad
            var interval = setInterval(function(){
                $<getAllVideoElements>().forEach(function(node) {
                    $<observeNode>(node);
                    $<notifyNode>(node);
                });

                $<getAllAudioElements>().forEach(function(node) {
                    $<observeNode>(node);
                    $<notifyNode>(node);
                });
            }, 1000);

            var timeout = setTimeout(function() {
                clearInterval(interval);
                clearTimeout(timeout);
            }, 10000);
        }

        $<sendMessage>({"pageLoad": false})
        $<observePage>();
    }
    
    
    // MARK: -----------------------------
    
    $<setupDetector>();
});
