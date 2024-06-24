/*
* Exceptions
* (C) 1999-2009,2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_EXCEPTION_H_
#define BOTAN_EXCEPTION_H_

#include <botan/types.h>
#include <exception>
#include <string>

namespace Botan {

/**
* Different types of errors that might occur
*/
enum class ErrorType {
   /** Some unknown error */
   Unknown = 1,
   /** An error while calling a system interface */
   SystemError,
   /** An operation seems valid, but not supported by the current version */
   NotImplemented,
   /** Memory allocation failure */
   OutOfMemory,
   /** An internal error occurred */
   InternalError,
   /** An I/O error occurred */
   IoError,

   /** Invalid object state */
   InvalidObjectState = 100,
   /** A key was not set on an object when this is required */
   KeyNotSet,
   /** The application provided an argument which is invalid */
   InvalidArgument,
   /** A key with invalid length was provided */
   InvalidKeyLength,
   /** A nonce with invalid length was provided */
   InvalidNonceLength,
   /** An object type was requested but cannot be found */
   LookupError,
   /** Encoding a message or datum failed */
   EncodingFailure,
   /** Decoding a message or datum failed */
   DecodingFailure,
   /** A TLS error (error_code will be the alert type) */
   TLSError,
   /** An error during an HTTP operation */
   HttpError,
   /** A message with an invalid authentication tag was detected */
   InvalidTag,
   /** An error during Roughtime validation */
   RoughtimeError,

   /** An error when interacting with CommonCrypto API */
   CommonCryptoError = 201,
   /** An error when interacting with a PKCS11 device */
   Pkcs11Error,
   /** An error when interacting with a TPM device */
   TPMError,
   /** An error when interacting with a database */
   DatabaseError,

   /** An error when interacting with zlib */
   ZlibError = 300,
   /** An error when interacting with bzip2 */
   Bzip2Error,
   /** An error when interacting with lzma */
   LzmaError,

};

//! \brief Convert an ErrorType to string
std::string BOTAN_PUBLIC_API(2, 11) to_string(ErrorType type);

/**
* Base class for all exceptions thrown by the library
*/
class BOTAN_PUBLIC_API(2, 0) Exception : public std::exception {
   public:
      /**
      * Return a descriptive string which is hopefully comprehensible to
      * a developer. It will likely not be useful for an end user.
      *
      * The string has no particular format, and the content of exception
      * messages may change from release to release. Thus the main use of this
      * function is for logging or debugging.
      */
      const char* what() const noexcept override { return m_msg.c_str(); }

      /**
      * Return the "type" of error which occurred.
      */
      virtual ErrorType error_type() const noexcept { return ErrorType::Unknown; }

      /**
      * Return an error code associated with this exception, or otherwise 0.
      *
      * The domain of this error varies depending on the source, for example on
      * POSIX systems it might be errno, while on a Windows system it might be
      * the result of GetLastError or WSAGetLastError.
      */
      virtual int error_code() const noexcept { return 0; }

      /**
      * Avoid throwing base Exception, use a subclass
      */
      explicit Exception(std::string_view msg);

      /**
      * Avoid throwing base Exception, use a subclass
      */
      Exception(const char* prefix, std::string_view msg);

      /**
      * Avoid throwing base Exception, use a subclass
      */
      Exception(std::string_view msg, const std::exception& e);

   private:
      std::string m_msg;
};

/**
* An invalid argument was provided to an API call.
*/
class BOTAN_PUBLIC_API(2, 0) Invalid_Argument : public Exception {
   public:
      explicit Invalid_Argument(std::string_view msg);

      explicit Invalid_Argument(std::string_view msg, std::string_view where);

      Invalid_Argument(std::string_view msg, const std::exception& e);

      ErrorType error_type() const noexcept override { return ErrorType::InvalidArgument; }
};

/**
* An invalid/unknown field name was passed to Public_Key::get_int_field
*/
class BOTAN_PUBLIC_API(3, 0) Unknown_PK_Field_Name final : public Invalid_Argument {
   public:
      Unknown_PK_Field_Name(std::string_view algo_name, std::string_view field_name);
};

/**
* An invalid key length was used
*/
class BOTAN_PUBLIC_API(2, 0) Invalid_Key_Length final : public Invalid_Argument {
   public:
      Invalid_Key_Length(std::string_view name, size_t length);

      ErrorType error_type() const noexcept override { return ErrorType::InvalidKeyLength; }
};

/**
* An invalid nonce length was used
*/
class BOTAN_PUBLIC_API(2, 0) Invalid_IV_Length final : public Invalid_Argument {
   public:
      Invalid_IV_Length(std::string_view mode, size_t bad_len);

      ErrorType error_type() const noexcept override { return ErrorType::InvalidNonceLength; }
};

/**
* Invalid_Algorithm_Name Exception
*/
class BOTAN_PUBLIC_API(2, 0) Invalid_Algorithm_Name final : public Invalid_Argument {
   public:
      explicit Invalid_Algorithm_Name(std::string_view name);
};

/**
* Encoding_Error Exception
*/
class BOTAN_PUBLIC_API(2, 0) Encoding_Error final : public Exception {
   public:
      explicit Encoding_Error(std::string_view name);

      ErrorType error_type() const noexcept override { return ErrorType::EncodingFailure; }
};

/**
* A decoding error occurred.
*/
class BOTAN_PUBLIC_API(2, 0) Decoding_Error : public Exception {
   public:
      explicit Decoding_Error(std::string_view name);

      Decoding_Error(std::string_view category, std::string_view err);

      Decoding_Error(std::string_view msg, const std::exception& e);

      ErrorType error_type() const noexcept override { return ErrorType::DecodingFailure; }
};

/**
* Invalid state was encountered. A request was made on an object while the
* object was in a state where the operation cannot be performed.
*/
class BOTAN_PUBLIC_API(2, 0) Invalid_State : public Exception {
   public:
      explicit Invalid_State(std::string_view err) : Exception(err) {}

      ErrorType error_type() const noexcept override { return ErrorType::InvalidObjectState; }
};

/**
* A PRNG was called on to produce output while still unseeded
*/
class BOTAN_PUBLIC_API(2, 0) PRNG_Unseeded final : public Invalid_State {
   public:
      explicit PRNG_Unseeded(std::string_view algo);
};

/**
* The key was not set on an object. This occurs with symmetric objects where
* an operation which requires the key is called prior to set_key being called.
*/
class BOTAN_PUBLIC_API(2, 4) Key_Not_Set : public Invalid_State {
   public:
      explicit Key_Not_Set(std::string_view algo);

      ErrorType error_type() const noexcept override { return ErrorType::KeyNotSet; }
};

/**
* A request was made for some kind of object which could not be located
*/
class BOTAN_PUBLIC_API(2, 0) Lookup_Error : public Exception {
   public:
      explicit Lookup_Error(std::string_view err) : Exception(err) {}

      Lookup_Error(std::string_view type, std::string_view algo, std::string_view provider = "");

      ErrorType error_type() const noexcept override { return ErrorType::LookupError; }
};

/**
* Algorithm_Not_Found Exception
*
* @warning This exception type will be removed in the future. Instead
* just catch Lookup_Error.
*/
class BOTAN_PUBLIC_API(2, 0) Algorithm_Not_Found final : public Lookup_Error {
   public:
      explicit Algorithm_Not_Found(std::string_view name);
};

/**
* Provider_Not_Found is thrown when a specific provider was requested
* but that provider is not available.
*
* @warning This exception type will be removed in the future. Instead
* just catch Lookup_Error.
*/
class BOTAN_PUBLIC_API(2, 0) Provider_Not_Found final : public Lookup_Error {
   public:
      Provider_Not_Found(std::string_view algo, std::string_view provider);
};

/**
* An AEAD or MAC check detected a message modification
*
* In versions before 2.10, Invalid_Authentication_Tag was named
* Integrity_Failure, it was renamed to make its usage more clear.
*/
class BOTAN_PUBLIC_API(2, 0) Invalid_Authentication_Tag final : public Exception {
   public:
      explicit Invalid_Authentication_Tag(std::string_view msg);

      ErrorType error_type() const noexcept override { return ErrorType::InvalidTag; }
};

/**
* For compatability with older versions
*/
typedef Invalid_Authentication_Tag Integrity_Failure;

/**
* An error occurred while operating on an IO stream
*/
class BOTAN_PUBLIC_API(2, 0) Stream_IO_Error final : public Exception {
   public:
      explicit Stream_IO_Error(std::string_view err);

      ErrorType error_type() const noexcept override { return ErrorType::IoError; }
};

/**
* System_Error
*
* This exception is thrown in the event of an error related to interacting
* with the operating system.
*
* This exception type also (optionally) captures an integer error code eg
* POSIX errno or Windows GetLastError.
*/
class BOTAN_PUBLIC_API(2, 9) System_Error : public Exception {
   public:
      System_Error(std::string_view msg) : Exception(msg), m_error_code(0) {}

      System_Error(std::string_view msg, int err_code);

      ErrorType error_type() const noexcept override { return ErrorType::SystemError; }

      int error_code() const noexcept override { return m_error_code; }

   private:
      int m_error_code;
};

/**
* An internal error occurred. If observed, please file a bug.
*/
class BOTAN_PUBLIC_API(2, 0) Internal_Error : public Exception {
   public:
      explicit Internal_Error(std::string_view err);

      ErrorType error_type() const noexcept override { return ErrorType::InternalError; }
};

/**
* Not Implemented Exception
*
* This is thrown in the situation where a requested operation is
* logically valid but is not implemented by this version of the library.
*/
class BOTAN_PUBLIC_API(2, 0) Not_Implemented final : public Exception {
   public:
      explicit Not_Implemented(std::string_view err);

      ErrorType error_type() const noexcept override { return ErrorType::NotImplemented; }
};

template <typename E, typename... Args>
inline void do_throw_error(const char* file, int line, const char* func, Args... args) {
   throw E(file, line, func, args...);
}

}  // namespace Botan

#endif
