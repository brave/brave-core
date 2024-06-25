/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/webgl/webgl2_rendering_context_base.h"
#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/renderer/bindings/modules/v8/webgl_any.h"

#include <algorithm>

using blink::ExecutionContext;
using blink::ScriptState;
using blink::ScriptValue;
using blink::WebGL2RenderingContextBase;
using blink::WebGLAny;

namespace {

ScriptValue FarbleGLIntParameter(WebGL2RenderingContextBase* owner,
                                 ScriptState* script_state,
                                 GLenum pname,
                                 int discard) {
  GLint value = 0;
  if (!owner->isContextLost())
    owner->ContextGL()->GetIntegerv(pname, &value);
  if (value > 0) {
    brave::FarblingPRNG prng =
        brave::BraveSessionCache::From(*ExecutionContext::From(script_state))
            .MakePseudoRandomGenerator();
    prng.discard(discard);
    if (prng() % 2 != 0) {
      value = value - 1;
    }
  }
  return WebGLAny(script_state, value);
}

ScriptValue FarbleGLInt64Parameter(WebGL2RenderingContextBase* owner,
                                   ScriptState* script_state,
                                   GLenum pname,
                                   int discard) {
  GLint64 value = 0;
  if (!owner->isContextLost())
    owner->ContextGL()->GetInteger64v(pname, &value);
  if (value > 0) {
    brave::FarblingPRNG prng =
        brave::BraveSessionCache::From(*ExecutionContext::From(script_state))
            .MakePseudoRandomGenerator();
    prng.discard(discard);
    if (prng() % 2 != 0) {
      value = value - 1;
    }
  }
  return WebGLAny(script_state, value);
}

}  // namespace

#define BRAVE_WEBGL2_RENDERING_CONTEXT_BASE                              \
  if (!brave::AllowFingerprinting(ExecutionContext::From(script_state))) \
    return ScriptValue::CreateNull(script_state->GetIsolate());

#define BRAVE_WEBGL2_RENDERING_CONTEXT_BASE_GETPARAMETER                \
  switch (brave::GetBraveFarblingLevelFor(                              \
      ExecutionContext::From(script_state), BraveFarblingLevel::OFF)) { \
    case BraveFarblingLevel::OFF: {                                     \
      break;                                                            \
    }                                                                   \
    case BraveFarblingLevel::MAXIMUM: {                                 \
      switch (pname) {                                                  \
        case GL_SHADING_LANGUAGE_VERSION:                               \
        case GL_VERSION:                                                \
        case GL_COPY_READ_BUFFER_BINDING:                               \
        case GL_COPY_WRITE_BUFFER_BINDING:                              \
        case GL_DRAW_FRAMEBUFFER_BINDING:                               \
        case GL_MAX_VERTEX_UNIFORM_COMPONENTS:                          \
        case GL_MAX_VERTEX_UNIFORM_BLOCKS:                              \
        case GL_MAX_VERTEX_OUTPUT_COMPONENTS:                           \
        case GL_MAX_VARYING_COMPONENTS:                                 \
        case GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS:          \
        case GL_MAX_FRAGMENT_UNIFORM_COMPONENTS:                        \
        case GL_MAX_FRAGMENT_UNIFORM_BLOCKS:                            \
        case GL_MAX_FRAGMENT_INPUT_COMPONENTS:                          \
        case GL_MAX_UNIFORM_BUFFER_BINDINGS:                            \
        case GL_MAX_COMBINED_UNIFORM_BLOCKS:                            \
        case GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS:                 \
        case GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS:               \
          return ScriptValue::CreateNull(script_state->GetIsolate());   \
      }                                                                 \
      break;                                                            \
    }                                                                   \
    case BraveFarblingLevel::BALANCED: {                                \
      switch (pname) {                                                  \
        case GL_MAX_VERTEX_UNIFORM_COMPONENTS:                          \
          return FarbleGLIntParameter(this, script_state, pname, 1);    \
        case GL_MAX_VERTEX_UNIFORM_BLOCKS:                              \
          return FarbleGLIntParameter(this, script_state, pname, 2);    \
        case GL_MAX_VERTEX_OUTPUT_COMPONENTS:                           \
          return FarbleGLIntParameter(this, script_state, pname, 3);    \
        case GL_MAX_VARYING_COMPONENTS:                                 \
          return FarbleGLIntParameter(this, script_state, pname, 4);    \
        case GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS:          \
          return FarbleGLIntParameter(this, script_state, pname, 5);    \
        case GL_MAX_FRAGMENT_UNIFORM_COMPONENTS:                        \
          return FarbleGLIntParameter(this, script_state, pname, 6);    \
        case GL_MAX_FRAGMENT_UNIFORM_BLOCKS:                            \
          return FarbleGLIntParameter(this, script_state, pname, 7);    \
        case GL_MAX_FRAGMENT_INPUT_COMPONENTS:                          \
          return FarbleGLIntParameter(this, script_state, pname, 8);    \
        case GL_MAX_UNIFORM_BUFFER_BINDINGS:                            \
          return FarbleGLIntParameter(this, script_state, pname, 9);    \
        case GL_MAX_COMBINED_UNIFORM_BLOCKS:                            \
          return FarbleGLIntParameter(this, script_state, pname, 10);   \
        case GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS:                 \
          return FarbleGLInt64Parameter(this, script_state, pname, 11); \
        case GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS:               \
          return FarbleGLInt64Parameter(this, script_state, pname, 12); \
      }                                                                 \
      break;                                                            \
    }                                                                   \
    default:                                                            \
      NOTREACHED_IN_MIGRATION();                                        \
  }

#include "src/third_party/blink/renderer/modules/webgl/webgl2_rendering_context_base.cc"
#undef BRAVE_WEBGL2_RENDERING_CONTEXT_BASE
#undef BRAVE_WEBGL2_RENDERING_CONTEXT_BASE_GETPARAMETER
