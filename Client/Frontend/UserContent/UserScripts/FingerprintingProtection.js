/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

if (webkit.messageHandlers.$<handler>) {
    (function () {
        function sendMessage(msg) {
            if (msg) {
                webkit.messageHandlers.$<handler>.postMessage(msg);
            }
        }

        let hooks = [
            {
                obj: window.CanvasRenderingContext2D.prototype,
                methods: ['getImageData', 'getLineDash', 'measureText']
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

    })();
}
