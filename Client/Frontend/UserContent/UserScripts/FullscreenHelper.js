/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


/*var $<possiblySupportedFunctions> = {
    request: ["requestFullscreen", "webkitRequestFullscreen", "webkitRequestFullScreen", "mozRequestFullScreen", "msRequestFullscreen"],
    exit: ["exitFullscreen", "webkitCancelFullScreen", "webkitExitFullscreen", "mozCancelFullScreen", "msExitFullscreen"],
    enabled: ["fullscreenEnabled", "webkitFullscreenEnabled", "mozFullScreenEnabled", "msFullscreenEnabled"],
    element: ["fullscreenElement", "webkitFullscreenElement", "webkitCurrentFullScreenElement", "mozFullScreenElement", "msFullscreenElement"],
    change: ["fullscreenchange", "webkitfullscreenchange", "mozfullscreenchange", "MSFullscreenChange"],
    error: ["fullscreenerror", "webkitfullscreenerror", "mozfullscreenerror", "MSFullscreenError"]
}*/

var $<isFullscreenSupportedNatively> = document.fullscreenEnabled ||
                                    document.webkitFullscreenEnabled ||
                                    document.mozFullScreenEnabled ||
                                    document.msFullscreenEnabled ? true : false;

var $<documentHasFullscreenFunctions> = document.documentElement.requestFullscreen ||
                                     document.documentElement.webkitRequestFullscreen ||
                                     document.documentElement.mozRequestFullScreen ||
                                     document.documentElement.msRequestFullscreen ||
                                     document.requestFullscreen ||
                                     document.webkitRequestFullscreen ||
                                     document.mozRequestFullScreen ||
                                     document.msRequestFullscreen ? true : false;

var $<videosSupportFullscreen> = HTMLVideoElement.prototype.webkitEnterFullscreen !== undefined;

if (!$<isFullscreenSupportedNatively> && $<videosSupportFullscreen>) {
    
    HTMLElement.prototype.requestFullscreen = function() {
        if (this.webkitRequestFullscreen !== undefined) {
            this.webkitRequestFullscreen();
            return true;
        }
        
        if (this.webkitEnterFullscreen !== undefined) {
            this.webkitEnterFullscreen();
            return true;
        }
        
        var video = this.querySelector("video")
        if (video !== undefined) {
            video.webkitEnterFullscreen();
            return true;
        }
        return false;
    };
    
    Object.defineProperty(document, 'fullscreenEnabled', {
        get: function() {
            return true;
        }
    });
    
    Object.defineProperty(document.documentElement, 'fullscreenEnabled', {
        get: function() {
            return true;
        }
    });
}
