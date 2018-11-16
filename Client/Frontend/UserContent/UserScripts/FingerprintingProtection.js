/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

if (webkit.messageHandlers.fingerprintingProtection) {
    install();
}

function install() {
    function sendMessage(msg) {
        if (msg) {
            webkit.messageHandlers.fingerprintingProtection.postMessage(msg);
        }
    }

    function installIFrameMethodTrap(obj, method) {
        let orig = obj[method];
        obj[method] = function () {
            var last = arguments[arguments.length - 1]
            if (last && last.toLowerCase() === 'canvas') {
                // Prevent fingerprinting using contentDocument.createElement('canvas'),
                // which evades trapInstanceMethod when the iframe is sandboxed
                sendMessage({ obj: `${obj}`, method: method })
            } else {
                // Otherwise apply the original method
                return orig.apply(this, arguments)
            }
        };
        return orig;
    }

    let hooks = [
        {
            obj: window.CanvasRenderingContext2D.prototype,
            methods: ['getImageData', 'getLineDash', 'measureText']
        },
        {
            obj: window.HTMLCanvasElement.prototype,
            methods: ['toDataURL', 'toBlob']
        },
        {
            obj: window.WebGLRenderingContext.prototype,
            methods: ['getSupportedExtensions', 'getParameter', 'getContextAttributes', 'getShaderPrecisionFormat', 'getExtension']
        },
        {
            obj: window.AudioBuffer.prototype,
            methods: ['copyFromChannel', 'getChannelData']
        },
        {
            obj: window.AnalyserNode.prototype,
            methods: ['getFloatFrequencyData', 'getByteFrequencyData', 'getFloatTimeDomainData', 'getByteTimeDomainData']
        }
    ];

    // Install Method Hooks
    hooks.forEach(function (hook) {
        hook.methods.forEach(function (method) {
            hook.obj[method] = function () {
                sendMessage({ obj: `${hook.obj}`, method: method });
            }
        });
    });

    // Install iframe Document Hooks
    if (window.frameElement) {
        installIFrameMethodTrap(document, 'createElement');
        installIFrameMethodTrap(document, 'createElementNS');
    }
}
