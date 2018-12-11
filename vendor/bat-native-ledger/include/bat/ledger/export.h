#ifndef BAT_LEDGER_EXPORT_H_
#define BAT_LEDGER_EXPORT_H_

#if defined(STANDALONE_BUILD)
#if defined(WIN32)

#if defined(BASE_IMPLEMENTATION)
#define LEDGER_EXPORT __declspec(dllexport)
#else
#define LEDGER_EXPORT __declspec(dllimport)
#endif  // defined(BASE_IMPLEMENTATION)

#else  // defined(WIN32)
#define LEDGER_EXPORT __attribute__((visibility("default")))
#endif

#else  // defined(STANDALONE_BUILD)
#define LEDGER_EXPORT
#endif

#endif  // BAT_LEDGER_EXPORT_H_
