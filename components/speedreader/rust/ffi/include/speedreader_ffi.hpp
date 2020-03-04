#ifndef SPEEDREADER_FFI_H
#define SPEEDREADER_FFI_H

#include <cstddef>
#include <cstdint>

namespace speedreader {

/// Indicate type of rewriter that would be used based on existing
/// configuration. `RewrtierUnknown` indicates that no configuration was found
/// for the provided parameters.
/// Also used to ask for a specific type of rewriter if desired; passing
/// `RewriterUnknown` tells SpeedReader to look the type up by configuration
/// and use heuristics-based one if not found otherwise.
enum class C_CRewriterType {
  RewriterStreaming,
  RewriterHeuristics,
  RewriterUnknown,
};

struct C_SpeedReader;

/// Opaque structure to have the minimum amount of type safety across the FFI.
/// Only replaces c_void
struct C_CRewriterOpaqueConfig {
  uint8_t _private[0];
};

/// Opaque structure to have the minimum amount of type safety across the FFI.
/// Only replaces c_void
struct C_CSpeedReaderRewriter {
  uint8_t _private[0];
};

struct C_CharBuf {
  const char *data;
  size_t len;
};

extern "C" {

/// Returns type of SpeedReader that would be applied by default for the given
/// URL. `RewriterUnknown` if no match in the whitelist.
C_CRewriterType speedreader_find_type(const C_SpeedReader *speedreader,
                                      const char *url,
                                      size_t url_len);

void speedreader_free(C_SpeedReader *speedreader);

void speedreader_free_rewriter_opaque_config(C_CRewriterOpaqueConfig *config);

C_CRewriterOpaqueConfig *speedreader_get_rewriter_opaque_config(const C_SpeedReader *speedreader,
                                                                const char *url,
                                                                size_t url_len);

/// New instance of SpeedReader. Loads the default configuration and rewriting
/// whitelists. Must be freed by calling `speedreader_free`.
C_SpeedReader *speedreader_new();

/// Complete rewriting for this instance.
/// Will free memory used by the rewriter.
/// Calling twice will cause panic.
int speedreader_rewriter_end(C_CSpeedReaderRewriter *rewriter);

void speedreader_rewriter_free(C_CSpeedReaderRewriter *rewriter);

/// Returns SpeedReader rewriter instance for the given URL. If provided
/// `rewriter_type` is `RewriterUnknown`, will look it up in the whitelist
/// and default to heuristics-based rewriter if none found in the whitelist.
/// Returns NULL if no URL provided or initialization fails.
/// Results of rewriting sent to `output_sink` callback function.
/// MUST be finished with `speedreader_rewriter_end` which will free
/// associated memory.
C_CSpeedReaderRewriter *speedreader_rewriter_new(const C_SpeedReader *speedreader,
                                                 const char *url,
                                                 size_t url_len,
                                                 void (*output_sink)(const char*, size_t, void*),
                                                 void *output_sink_user_data,
                                                 C_CRewriterOpaqueConfig *rewriter_opaque_config,
                                                 C_CRewriterType rewriter_type);

/// Write a new chunk of data (byte array) to the rewriter instance.
int speedreader_rewriter_write(C_CSpeedReaderRewriter *rewriter,
                               const char *chunk,
                               size_t chunk_len);

void speedreader_str_free(C_CharBuf string);

const C_CharBuf *speedreader_take_last_error();

/// Checks if the provided URL matches whitelisted readable URLs.
bool speedreader_url_readable(const C_SpeedReader *speedreader,
                              const char *url,
                              size_t url_len);

/// New instance of SpeedReader using deserialized whitelist
C_SpeedReader *speedreader_with_whitelist(const char *whitelist_data,
                                          size_t whitelist_data_size);

} // extern "C"

} // namespace speedreader

#endif // SPEEDREADER_FFI_H
