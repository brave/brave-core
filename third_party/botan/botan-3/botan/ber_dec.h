/*
* BER Decoder
* (C) 1999-2010,2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_BER_DECODER_H_
#define BOTAN_BER_DECODER_H_

#include <botan/asn1_obj.h>
#include <botan/data_src.h>
#include <botan/mem_ops.h>

namespace Botan {

class BigInt;

/**
* BER Decoding Object
*/
class BOTAN_PUBLIC_API(2, 0) BER_Decoder final {
   public:
      /**
      * Set up to BER decode the data in buf of length len
      */
      BER_Decoder(const uint8_t buf[], size_t len);

      /**
      * Set up to BER decode the data in buf of length len
      */
      BER_Decoder(std::span<const uint8_t> buf) : BER_Decoder(buf.data(), buf.size()) {}

      /**
      * Set up to BER decode the data in vec
      */
      explicit BER_Decoder(const secure_vector<uint8_t>& vec);

      /**
      * Set up to BER decode the data in vec
      */
      explicit BER_Decoder(const std::vector<uint8_t>& vec);

      /**
      * Set up to BER decode the data in src
      */
      explicit BER_Decoder(DataSource& src);

      /**
      * Set up to BER decode the data in obj
      */
      BER_Decoder(const BER_Object& obj) : BER_Decoder(obj.bits(), obj.length()) {}

      /**
      * Set up to BER decode the data in obj
      */
      BER_Decoder(BER_Object&& obj) : BER_Decoder(std::move(obj), nullptr) {}

      BER_Decoder(const BER_Decoder& other);

      BER_Decoder& operator=(const BER_Decoder&) = delete;

      /**
      * Get the next object in the data stream.
      * If EOF, returns an object with type NO_OBJECT.
      */
      BER_Object get_next_object();

      BER_Decoder& get_next(BER_Object& ber) {
         ber = get_next_object();
         return (*this);
      }

      /**
      * Peek at the next object without removing it from the stream
      *
      * If an object has been pushed, then it returns that object.
      * Otherwise it reads the next object and pushes it. Thus, a you
      * call peek_next_object followed by push_back without a
      * subsequent read, it will fail.
      */
      const BER_Object& peek_next_object();

      /**
      * Push an object back onto the stream. Throws if another
      * object was previously pushed and has not been subsequently
      * read out.
      */
      void push_back(const BER_Object& obj);

      /**
      * Push an object back onto the stream. Throws if another
      * object was previously pushed and has not been subsequently
      * read out.
      */
      void push_back(BER_Object&& obj);

      /**
      * Return true if there is at least one more item remaining
      */
      bool more_items() const;

      /**
      * Verify the stream is concluded, throws otherwise.
      * Returns (*this)
      */
      BER_Decoder& verify_end();

      /**
      * Verify the stream is concluded, throws otherwise.
      * Returns (*this)
      */
      BER_Decoder& verify_end(std::string_view err_msg);

      /**
      * Discard any data that remains unread
      * Returns (*this)
      */
      BER_Decoder& discard_remaining();

      BER_Decoder start_cons(ASN1_Type type_tag, ASN1_Class class_tag);

      BER_Decoder start_sequence() { return start_cons(ASN1_Type::Sequence, ASN1_Class::Universal); }

      BER_Decoder start_set() { return start_cons(ASN1_Type::Set, ASN1_Class::Universal); }

      BER_Decoder start_context_specific(uint32_t tag) {
         return start_cons(ASN1_Type(tag), ASN1_Class::ContextSpecific);
      }

      BER_Decoder start_explicit_context_specific(uint32_t tag) {
         return start_cons(ASN1_Type(tag), ASN1_Class::ExplicitContextSpecific);
      }

      /**
      * Finish decoding a constructed data, throws if any data remains.
      * Returns the parent of *this (ie the object on which start_cons was called).
      */
      BER_Decoder& end_cons();

      /**
      * Get next object and copy value to POD type
      * Asserts value length is equal to POD type sizeof.
      * Asserts Type tag and optional Class tag according to parameters.
      * Copy value to POD type (struct, union, C-style array, std::array, etc.).
      * @param out POD type reference where to copy object value
      * @param type_tag ASN1_Type enum to assert type on object read
      * @param class_tag ASN1_Type enum to assert class on object read (default: CONTEXT_SPECIFIC)
      * @return this reference
      */
      template <typename T>
      BER_Decoder& get_next_value(T& out, ASN1_Type type_tag, ASN1_Class class_tag = ASN1_Class::ContextSpecific)
         requires std::is_standard_layout<T>::value && std::is_trivial<T>::value
      {
         BER_Object obj = get_next_object();
         obj.assert_is_a(type_tag, class_tag);

         if(obj.length() != sizeof(T)) {
            throw BER_Decoding_Error("Size mismatch. Object value size is " + std::to_string(obj.length()) +
                                     "; Output type size is " + std::to_string(sizeof(T)));
         }

         copy_mem(reinterpret_cast<uint8_t*>(&out), obj.bits(), obj.length());

         return (*this);
      }

      /*
      * Save all the bytes remaining in the source
      */
      template <typename Alloc>
      BER_Decoder& raw_bytes(std::vector<uint8_t, Alloc>& out) {
         out.clear();
         uint8_t buf;
         while(m_source->read_byte(buf)) {
            out.push_back(buf);
         }
         return (*this);
      }

      BER_Decoder& decode_null();

      /**
      * Decode a BER encoded BOOLEAN
      */
      BER_Decoder& decode(bool& out) { return decode(out, ASN1_Type::Boolean, ASN1_Class::Universal); }

      /*
      * Decode a small BER encoded INTEGER
      */
      BER_Decoder& decode(size_t& out) { return decode(out, ASN1_Type::Integer, ASN1_Class::Universal); }

      /*
      * Decode a BER encoded INTEGER
      */
      BER_Decoder& decode(BigInt& out) { return decode(out, ASN1_Type::Integer, ASN1_Class::Universal); }

      std::vector<uint8_t> get_next_octet_string() {
         std::vector<uint8_t> out_vec;
         decode(out_vec, ASN1_Type::OctetString);
         return out_vec;
      }

      /*
      * BER decode a BIT STRING or OCTET STRING
      */
      template <typename Alloc>
      BER_Decoder& decode(std::vector<uint8_t, Alloc>& out, ASN1_Type real_type) {
         return decode(out, real_type, real_type, ASN1_Class::Universal);
      }

      BER_Decoder& decode(bool& v, ASN1_Type type_tag, ASN1_Class class_tag = ASN1_Class::ContextSpecific);

      BER_Decoder& decode(size_t& v, ASN1_Type type_tag, ASN1_Class class_tag = ASN1_Class::ContextSpecific);

      BER_Decoder& decode(BigInt& v, ASN1_Type type_tag, ASN1_Class class_tag = ASN1_Class::ContextSpecific);

      BER_Decoder& decode(std::vector<uint8_t>& v,
                          ASN1_Type real_type,
                          ASN1_Type type_tag,
                          ASN1_Class class_tag = ASN1_Class::ContextSpecific);

      BER_Decoder& decode(secure_vector<uint8_t>& v,
                          ASN1_Type real_type,
                          ASN1_Type type_tag,
                          ASN1_Class class_tag = ASN1_Class::ContextSpecific);

      BER_Decoder& decode(ASN1_Object& obj,
                          ASN1_Type type_tag = ASN1_Type::NoObject,
                          ASN1_Class class_tag = ASN1_Class::NoObject);

      /**
      * Decode an integer value which is typed as an octet string
      */
      BER_Decoder& decode_octet_string_bigint(BigInt& b);

      uint64_t decode_constrained_integer(ASN1_Type type_tag, ASN1_Class class_tag, size_t T_bytes);

      template <typename T>
      BER_Decoder& decode_integer_type(T& out) {
         return decode_integer_type<T>(out, ASN1_Type::Integer, ASN1_Class::Universal);
      }

      template <typename T>
      BER_Decoder& decode_integer_type(T& out, ASN1_Type type_tag, ASN1_Class class_tag = ASN1_Class::ContextSpecific) {
         out = static_cast<T>(decode_constrained_integer(type_tag, class_tag, sizeof(out)));
         return (*this);
      }

      template <typename T>
      BER_Decoder& decode_optional(T& out, ASN1_Type type_tag, ASN1_Class class_tag, const T& default_value = T());

      template <typename T>
      BER_Decoder& decode_optional_implicit(T& out,
                                            ASN1_Type type_tag,
                                            ASN1_Class class_tag,
                                            ASN1_Type real_type,
                                            ASN1_Class real_class,
                                            const T& default_value = T());

      template <typename T>
      BER_Decoder& decode_list(std::vector<T>& out,
                               ASN1_Type type_tag = ASN1_Type::Sequence,
                               ASN1_Class class_tag = ASN1_Class::Universal);

      template <typename T>
      bool decode_optional_list(std::vector<T>& out,
                                ASN1_Type type_tag = ASN1_Type::Sequence,
                                ASN1_Class class_tag = ASN1_Class::Universal);

      template <typename T>
      BER_Decoder& decode_and_check(const T& expected, std::string_view error_msg) {
         T actual;
         decode(actual);

         if(actual != expected) {
            throw Decoding_Error(error_msg);
         }

         return (*this);
      }

      /*
      * Decode an OPTIONAL string type
      */
      template <typename Alloc>
      BER_Decoder& decode_optional_string(std::vector<uint8_t, Alloc>& out,
                                          ASN1_Type real_type,
                                          uint32_t expected_tag,
                                          ASN1_Class class_tag = ASN1_Class::ContextSpecific) {
         BER_Object obj = get_next_object();

         ASN1_Type type_tag = static_cast<ASN1_Type>(expected_tag);

         if(obj.is_a(type_tag, class_tag)) {
            if(class_tag == ASN1_Class::ExplicitContextSpecific) {
               BER_Decoder(std::move(obj)).decode(out, real_type).verify_end();
            } else {
               push_back(std::move(obj));
               decode(out, real_type, type_tag, class_tag);
            }
         } else {
            out.clear();
            push_back(std::move(obj));
         }

         return (*this);
      }

      template <typename Alloc>
      BER_Decoder& decode_optional_string(std::vector<uint8_t, Alloc>& out,
                                          ASN1_Type real_type,
                                          ASN1_Type expected_tag,
                                          ASN1_Class class_tag = ASN1_Class::ContextSpecific) {
         return decode_optional_string(out, real_type, static_cast<uint32_t>(expected_tag), class_tag);
      }

   private:
      BER_Decoder(BER_Object&& obj, BER_Decoder* parent);

      BER_Decoder* m_parent = nullptr;
      BER_Object m_pushed;
      // either m_data_src.get() or an unowned pointer
      DataSource* m_source;
      mutable std::unique_ptr<DataSource> m_data_src;
};

/*
* Decode an OPTIONAL or DEFAULT element
*/
template <typename T>
BER_Decoder& BER_Decoder::decode_optional(T& out, ASN1_Type type_tag, ASN1_Class class_tag, const T& default_value) {
   BER_Object obj = get_next_object();

   if(obj.is_a(type_tag, class_tag)) {
      if(class_tag == ASN1_Class::ExplicitContextSpecific) {
         BER_Decoder(std::move(obj)).decode(out).verify_end();
      } else {
         push_back(std::move(obj));
         decode(out, type_tag, class_tag);
      }
   } else {
      out = default_value;
      push_back(std::move(obj));
   }

   return (*this);
}

/*
* Decode an OPTIONAL or DEFAULT element
*/
template <typename T>
BER_Decoder& BER_Decoder::decode_optional_implicit(T& out,
                                                   ASN1_Type type_tag,
                                                   ASN1_Class class_tag,
                                                   ASN1_Type real_type,
                                                   ASN1_Class real_class,
                                                   const T& default_value) {
   BER_Object obj = get_next_object();

   if(obj.is_a(type_tag, class_tag)) {
      obj.set_tagging(real_type, real_class);
      push_back(std::move(obj));
      decode(out, real_type, real_class);
   } else {
      // Not what we wanted, push it back on the stream
      out = default_value;
      push_back(std::move(obj));
   }

   return (*this);
}

/*
* Decode a list of homogenously typed values
*/
template <typename T>
BER_Decoder& BER_Decoder::decode_list(std::vector<T>& vec, ASN1_Type type_tag, ASN1_Class class_tag) {
   BER_Decoder list = start_cons(type_tag, class_tag);

   while(list.more_items()) {
      T value;
      list.decode(value);
      vec.push_back(std::move(value));
   }

   list.end_cons();

   return (*this);
}

/*
* Decode an optional list of homogenously typed values
*/
template <typename T>
bool BER_Decoder::decode_optional_list(std::vector<T>& vec, ASN1_Type type_tag, ASN1_Class class_tag) {
   if(peek_next_object().is_a(type_tag, class_tag)) {
      decode_list(vec, type_tag, class_tag);
      return true;
   }

   return false;
}

}  // namespace Botan

#endif
