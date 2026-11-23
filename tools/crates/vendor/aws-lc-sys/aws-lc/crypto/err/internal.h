// Copyright (c) 2017, Google Inc.
// SPDX-License-Identifier: ISC

#ifndef OPENSSL_HEADER_CRYPTO_ERR_INTERNAL_H
#define OPENSSL_HEADER_CRYPTO_ERR_INTERNAL_H

#include <openssl/err.h>

#if defined(__cplusplus)
extern "C" {
#endif


// Private error queue functions.

// ERR_SAVE_STATE contains a saved representation of the error queue. It is
// slightly more compact than |ERR_STATE| as the error queue will typically not
// contain |ERR_NUM_ERRORS| entries.
typedef struct err_save_state_st ERR_SAVE_STATE;

// ERR_SAVE_STATE_free releases all memory associated with |state|.
OPENSSL_EXPORT void ERR_SAVE_STATE_free(ERR_SAVE_STATE *state);

// ERR_save_state returns a newly-allocated |ERR_SAVE_STATE| structure
// containing the current state of the error queue or NULL on allocation
// error. It should be released with |ERR_SAVE_STATE_free|.
OPENSSL_EXPORT ERR_SAVE_STATE *ERR_save_state(void);

// ERR_restore_state clears the error queue and replaces it with |state|. Marks
// are not part of a saved state: they are neither captured nor restored.
OPENSSL_EXPORT void ERR_restore_state(const ERR_SAVE_STATE *state);

// ERR_suppress_errors_begin makes the current thread's error queue reject new
// errors until the matching |ERR_suppress_errors_end|. Use it around
// best-effort work whose failures are not the caller's business.
//
// It returns one on success and zero if the thread has no error state and one
// cannot be allocated, in which case nothing is suppressed: skip the work rather
// than run it unprotected, and do not call |ERR_suppress_errors_end|.
//
// The queue is left bit-for-bit as it was found, so the caller keeps its
// entries, its marks, and the data strings it has already read out of them.
// Rejecting the errors is what makes that hold once the queue saturates, where
// every new error evicts one of the caller's and trimming the queue afterward
// cannot bring an evicted entry back.
//
// This preservation guarantee requires that the entire scoped call graph
// neither modifies caller-owned queue state nor relies on reading its own
// errors. Only new errors and error-data writes are suppressed; other queue
// operations, including |ERR_get_error|, |ERR_clear_error|, |ERR_set_mark|,
// |ERR_pop_to_mark|, and |ERR_restore_state|, still reach caller-owned state.
//
// |ERR_save_state| and |ERR_restore_state| can recover saved error entries,
// but do not preserve marks or the lifetime of previously returned error-data
// pointers. They also do not fix error-driven behavior inside the scope.
//
// Scopes nest, and errors raised outside them are unaffected.
OPENSSL_EXPORT int ERR_suppress_errors_begin(void);

// ERR_suppress_errors_end ends the innermost |ERR_suppress_errors_begin| scope.
// Call it only after a begin that returned one.
OPENSSL_EXPORT void ERR_suppress_errors_end(void);


#if defined(__cplusplus)
}  // extern C

extern "C++" {

BSSL_NAMESPACE_BEGIN

BORINGSSL_MAKE_DELETER(ERR_SAVE_STATE, ERR_SAVE_STATE_free)

// ScopedErrorSuppression holds an |ERR_suppress_errors_begin| scope for as long
// as it is alive. It converts to false when the scope could not be opened, where
// the guarded work is to be skipped rather than run unsuppressed.
class ScopedErrorSuppression {
 public:
  ScopedErrorSuppression() : active_(ERR_suppress_errors_begin() != 0) {}
  ~ScopedErrorSuppression() {
    if (active_) {
      ERR_suppress_errors_end();
    }
  }
  ScopedErrorSuppression(const ScopedErrorSuppression &) = delete;
  ScopedErrorSuppression &operator=(const ScopedErrorSuppression &) = delete;

  explicit operator bool() const { return active_; }

 private:
  const bool active_;
};

BSSL_NAMESPACE_END

}  // extern C++
#endif

#endif  // OPENSSL_HEADER_CRYPTO_ERR_INTERNAL_H
