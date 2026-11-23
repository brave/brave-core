/*
 * Copyright (c) The mldsa-native project authors
 * SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT
 */
#ifndef MLD_CONTEXT_H
#define MLD_CONTEXT_H

/* This header is included by common.h once the configuration has been pulled
 * in; it is not meant to be included directly. */
#if !defined(__ASSEMBLER__)

#include <stdint.h>
#include "cbmc.h"
#include "sys.h"

/*
 * If the integration wants to provide a context parameter for use in
 * platform-specific hooks, then it should define this parameter.
 *
 * The MLD_CONTEXT_PARAMETERS_n macros are intended to be used with macros
 * defining the function names and expand to either pass or discard the context
 * argument as required by the current build.  If there is no context parameter
 * requested then these are removed from the prototypes and from all calls.
 */
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
#define MLD_CONTEXT_PARAMETERS_0(context) (context)
#define MLD_CONTEXT_PARAMETERS_1(arg0, context) (arg0, context)
#define MLD_CONTEXT_PARAMETERS_2(arg0, arg1, context) (arg0, arg1, context)
#define MLD_CONTEXT_PARAMETERS_3(arg0, arg1, arg2, context) \
  (arg0, arg1, arg2, context)
#define MLD_CONTEXT_PARAMETERS_4(arg0, arg1, arg2, arg3, context) \
  (arg0, arg1, arg2, arg3, context)
#define MLD_CONTEXT_PARAMETERS_5(arg0, arg1, arg2, arg3, arg4, context) \
  (arg0, arg1, arg2, arg3, arg4, context)
#define MLD_CONTEXT_PARAMETERS_6(arg0, arg1, arg2, arg3, arg4, arg5, context) \
  (arg0, arg1, arg2, arg3, arg4, arg5, context)
#define MLD_CONTEXT_PARAMETERS_7(arg0, arg1, arg2, arg3, arg4, arg5, arg6, \
                                 context)                                  \
  (arg0, arg1, arg2, arg3, arg4, arg5, arg6, context)
#define MLD_CONTEXT_PARAMETERS_8(arg0, arg1, arg2, arg3, arg4, arg5, arg6, \
                                 arg7, context)                            \
  (arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, context)
#define MLD_CONTEXT_PARAMETERS_9(arg0, arg1, arg2, arg3, arg4, arg5, arg6, \
                                 arg7, arg8, context)                      \
  (arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, context)
#else /* MLD_CONFIG_CONTEXT_PARAMETER */
#define MLD_CONTEXT_PARAMETERS_0(context) ()
#define MLD_CONTEXT_PARAMETERS_1(arg0, context) (arg0)
#define MLD_CONTEXT_PARAMETERS_2(arg0, arg1, context) (arg0, arg1)
#define MLD_CONTEXT_PARAMETERS_3(arg0, arg1, arg2, context) (arg0, arg1, arg2)
#define MLD_CONTEXT_PARAMETERS_4(arg0, arg1, arg2, arg3, context) \
  (arg0, arg1, arg2, arg3)
#define MLD_CONTEXT_PARAMETERS_5(arg0, arg1, arg2, arg3, arg4, context) \
  (arg0, arg1, arg2, arg3, arg4)
#define MLD_CONTEXT_PARAMETERS_6(arg0, arg1, arg2, arg3, arg4, arg5, context) \
  (arg0, arg1, arg2, arg3, arg4, arg5)
#define MLD_CONTEXT_PARAMETERS_7(arg0, arg1, arg2, arg3, arg4, arg5, arg6, \
                                 context)                                  \
  (arg0, arg1, arg2, arg3, arg4, arg5, arg6)
#define MLD_CONTEXT_PARAMETERS_8(arg0, arg1, arg2, arg3, arg4, arg5, arg6, \
                                 arg7, context)                            \
  (arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define MLD_CONTEXT_PARAMETERS_9(arg0, arg1, arg2, arg3, arg4, arg5, arg6, \
                                 arg7, arg8, context)                      \
  (arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
#endif /* !MLD_CONFIG_CONTEXT_PARAMETER */

/* Consume a context parameter carried only for the integration's benefit,
 * avoiding -Wunused-parameter; expands to nothing when no context is
 * configured. */
#if defined(MLD_CONFIG_CONTEXT_PARAMETER)
#define MLD_CONTEXT_UNUSED(context) ((void)(context))
#else
#define MLD_CONTEXT_UNUSED(context) ((void)0)
#endif

#if defined(MLD_CONFIG_CONTEXT_PARAMETER_TYPE) != \
    defined(MLD_CONFIG_CONTEXT_PARAMETER)
#error MLD_CONFIG_CONTEXT_PARAMETER_TYPE must be defined if and only if MLD_CONFIG_CONTEXT_PARAMETER is defined
#endif

/* The signing hooks tie into the rejection-sampling loop. A pausing attempt
 * hook only reproduces the uninterrupted signature if the randomness is fixed
 * across calls, and thus requires the deterministic API.
 * For now we impose that requirement on all three hooks uniformly: enabling any
 * of them requires MLD_CONFIG_NO_RANDOMIZED_API. This also rules out
 * MLD_CONFIG_KEYGEN_PCT (whose PCT needs the randomized signature(), see
 * common.h).
 *
 * A logging-only use (attempt always returns 0; resume/finish merely observe)
 * would be safe with the randomized API too, but the restriction is applied
 * uniformly for now. */
#if (defined(MLD_CONFIG_SIGN_HOOK_RESUME) ||  \
     defined(MLD_CONFIG_SIGN_HOOK_ATTEMPT) || \
     defined(MLD_CONFIG_SIGN_HOOK_FINISH)) && \
    !defined(MLD_CONFIG_NO_RANDOMIZED_API)
#error Signing hooks (MLD_CONFIG_SIGN_HOOK_RESUME / _ATTEMPT / _FINISH) require MLD_CONFIG_NO_RANDOMIZED_API
#endif /* (MLD_CONFIG_SIGN_HOOK_RESUME || MLD_CONFIG_SIGN_HOOK_ATTEMPT || \
          MLD_CONFIG_SIGN_HOOK_FINISH) && !MLD_CONFIG_NO_RANDOMIZED_API */

/* Signing hooks (MLD_CONFIG_SIGN_HOOK_RESUME / _ATTEMPT / _FINISH; documented
 * in mldsa_native_config.h). The following macros route the call sites to
 * mld_sign_hook_*, appending or dropping the context argument; each unset hook
 * uses the dummy below. */
#define mld_sign_resume mld_sign_hook_resume MLD_CONTEXT_PARAMETERS_0
#define mld_sign_attempt mld_sign_hook_attempt MLD_CONTEXT_PARAMETERS_1
#define mld_sign_finish mld_sign_hook_finish MLD_CONTEXT_PARAMETERS_1

/* We don't use mld_sign_resume here because MLD_CONTEXT_PARAMETERS_0 is
 * unsuitable for function declarations: it misses `void` as the placeholder
 * argument. */
#if !defined(MLD_CONFIG_SIGN_HOOK_RESUME)
MLD_MUST_CHECK_RETURN_VALUE
static MLD_INLINE uint16_t mld_sign_hook_resume(
#if defined(MLD_CONFIG_CONTEXT_PARAMETER)
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#else
    void
#endif
)
__contract__(assigns() ensures(1))
{
  MLD_CONTEXT_UNUSED(context);
  return 0;
}
#endif /* !MLD_CONFIG_SIGN_HOOK_RESUME */

#if !defined(MLD_CONFIG_SIGN_HOOK_ATTEMPT)
MLD_MUST_CHECK_RETURN_VALUE
static MLD_INLINE int mld_sign_attempt(
    uint16_t attempt, MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(assigns() ensures(1))
{
  ((void)attempt);
  MLD_CONTEXT_UNUSED(context);
  return 0;
}
#endif /* !MLD_CONFIG_SIGN_HOOK_ATTEMPT */

#if !defined(MLD_CONFIG_SIGN_HOOK_FINISH)
static MLD_INLINE void mld_sign_finish(
    uint16_t attempt, MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(assigns() ensures(1))
{
  ((void)attempt);
  MLD_CONTEXT_UNUSED(context);
}
#endif /* !MLD_CONFIG_SIGN_HOOK_FINISH */

#endif /* !__ASSEMBLER__ */

#endif /* !MLD_CONTEXT_H */
