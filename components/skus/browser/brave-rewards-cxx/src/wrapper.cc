#include "shim.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace rust {
inline namespace cxxbridge1 {
// #include "rust/cxx.h"

#ifndef CXXBRIDGE1_PANIC
#define CXXBRIDGE1_PANIC
template <typename Exception>
void panic [[noreturn]] (const char *msg);
#endif // CXXBRIDGE1_PANIC

struct unsafe_bitcopy_t;

namespace {
template <typename T>
class impl;
} // namespace

template <typename T>
::std::size_t size_of();
template <typename T>
::std::size_t align_of();

#ifndef CXXBRIDGE1_RUST_STRING
#define CXXBRIDGE1_RUST_STRING
class String final {
public:
  String() noexcept;
  String(const String &) noexcept;
  String(String &&) noexcept;
  ~String() noexcept;

  String(const std::string &);
  String(const char *);
  String(const char *, std::size_t);
  String(const char16_t *);
  String(const char16_t *, std::size_t);

  String &operator=(const String &) &noexcept;
  String &operator=(String &&) &noexcept;

  explicit operator std::string() const;

  const char *data() const noexcept;
  std::size_t size() const noexcept;
  std::size_t length() const noexcept;
  bool empty() const noexcept;

  const char *c_str() noexcept;

  std::size_t capacity() const noexcept;
  void reserve(size_t new_cap) noexcept;

  using iterator = char *;
  iterator begin() noexcept;
  iterator end() noexcept;

  using const_iterator = const char *;
  const_iterator begin() const noexcept;
  const_iterator end() const noexcept;
  const_iterator cbegin() const noexcept;
  const_iterator cend() const noexcept;

  bool operator==(const String &) const noexcept;
  bool operator!=(const String &) const noexcept;
  bool operator<(const String &) const noexcept;
  bool operator<=(const String &) const noexcept;
  bool operator>(const String &) const noexcept;
  bool operator>=(const String &) const noexcept;

  void swap(String &) noexcept;

  String(unsafe_bitcopy_t, const String &) noexcept;

private:
  friend void swap(String &lhs, String &rhs) noexcept { lhs.swap(rhs); }

  std::array<std::uintptr_t, 3> repr;
};
#endif // CXXBRIDGE1_RUST_STRING

#ifndef CXXBRIDGE1_RUST_STR
#define CXXBRIDGE1_RUST_STR
class Str final {
public:
  Str() noexcept;
  Str(const String &) noexcept;
  Str(const std::string &);
  Str(const char *);
  Str(const char *, std::size_t);

  Str &operator=(const Str &) &noexcept = default;

  explicit operator std::string() const;

  const char *data() const noexcept;
  std::size_t size() const noexcept;
  std::size_t length() const noexcept;
  bool empty() const noexcept;

  Str(const Str &) noexcept = default;
  ~Str() noexcept = default;

  using iterator = const char *;
  using const_iterator = const char *;
  const_iterator begin() const noexcept;
  const_iterator end() const noexcept;
  const_iterator cbegin() const noexcept;
  const_iterator cend() const noexcept;

  bool operator==(const Str &) const noexcept;
  bool operator!=(const Str &) const noexcept;
  bool operator<(const Str &) const noexcept;
  bool operator<=(const Str &) const noexcept;
  bool operator>(const Str &) const noexcept;
  bool operator>=(const Str &) const noexcept;

  void swap(Str &) noexcept;

private:
  class uninit;
  Str(uninit) noexcept;
  friend impl<Str>;

  std::array<std::uintptr_t, 2> repr;
};
#endif // CXXBRIDGE1_RUST_STR

#ifndef CXXBRIDGE1_RUST_SLICE
#define CXXBRIDGE1_RUST_SLICE
namespace detail {
template <bool>
struct copy_assignable_if {};

template <>
struct copy_assignable_if<false> {
  copy_assignable_if() noexcept = default;
  copy_assignable_if(const copy_assignable_if &) noexcept = default;
  copy_assignable_if &operator=(const copy_assignable_if &) &noexcept = delete;
  copy_assignable_if &operator=(copy_assignable_if &&) &noexcept = default;
};
} // namespace detail

template <typename T>
class Slice final
    : private detail::copy_assignable_if<std::is_const<T>::value> {
public:
  using value_type = T;

  Slice() noexcept;
  Slice(T *, std::size_t count) noexcept;

  Slice &operator=(const Slice<T> &) &noexcept = default;
  Slice &operator=(Slice<T> &&) &noexcept = default;

  T *data() const noexcept;
  std::size_t size() const noexcept;
  std::size_t length() const noexcept;
  bool empty() const noexcept;

  T &operator[](std::size_t n) const noexcept;
  T &at(std::size_t n) const;
  T &front() const noexcept;
  T &back() const noexcept;

  Slice(const Slice<T> &) noexcept = default;
  ~Slice() noexcept = default;

  class iterator;
  iterator begin() const noexcept;
  iterator end() const noexcept;

  void swap(Slice &) noexcept;

private:
  class uninit;
  Slice(uninit) noexcept;
  friend impl<Slice>;
  friend void sliceInit(void *, const void *, std::size_t) noexcept;
  friend void *slicePtr(const void *) noexcept;
  friend std::size_t sliceLen(const void *) noexcept;

  std::array<std::uintptr_t, 2> repr;
};

template <typename T>
class Slice<T>::iterator final {
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = typename std::add_pointer<T>::type;
  using reference = typename std::add_lvalue_reference<T>::type;

  reference operator*() const noexcept;
  pointer operator->() const noexcept;
  reference operator[](difference_type) const noexcept;

  iterator &operator++() noexcept;
  iterator operator++(int) noexcept;
  iterator &operator--() noexcept;
  iterator operator--(int) noexcept;

  iterator &operator+=(difference_type) noexcept;
  iterator &operator-=(difference_type) noexcept;
  iterator operator+(difference_type) const noexcept;
  iterator operator-(difference_type) const noexcept;
  difference_type operator-(const iterator &) const noexcept;

  bool operator==(const iterator &) const noexcept;
  bool operator!=(const iterator &) const noexcept;
  bool operator<(const iterator &) const noexcept;
  bool operator<=(const iterator &) const noexcept;
  bool operator>(const iterator &) const noexcept;
  bool operator>=(const iterator &) const noexcept;

private:
  friend class Slice;
  void *pos;
  std::size_t stride;
};

template <typename T>
Slice<T>::Slice() noexcept {
  sliceInit(this, reinterpret_cast<void *>(align_of<T>()), 0);
}

template <typename T>
Slice<T>::Slice(T *s, std::size_t count) noexcept {
  assert(s != nullptr || count == 0);
  sliceInit(this,
            s == nullptr && count == 0
                ? reinterpret_cast<void *>(align_of<T>())
                : const_cast<typename std::remove_const<T>::type *>(s),
            count);
}

template <typename T>
T *Slice<T>::data() const noexcept {
  return reinterpret_cast<T *>(slicePtr(this));
}

template <typename T>
std::size_t Slice<T>::size() const noexcept {
  return sliceLen(this);
}

template <typename T>
std::size_t Slice<T>::length() const noexcept {
  return this->size();
}

template <typename T>
bool Slice<T>::empty() const noexcept {
  return this->size() == 0;
}

template <typename T>
T &Slice<T>::operator[](std::size_t n) const noexcept {
  assert(n < this->size());
  auto ptr = static_cast<char *>(slicePtr(this)) + size_of<T>() * n;
  return *reinterpret_cast<T *>(ptr);
}

template <typename T>
T &Slice<T>::at(std::size_t n) const {
  if (n >= this->size()) {
    panic<std::out_of_range>("rust::Slice index out of range");
  }
  return (*this)[n];
}

template <typename T>
T &Slice<T>::front() const noexcept {
  assert(!this->empty());
  return (*this)[0];
}

template <typename T>
T &Slice<T>::back() const noexcept {
  assert(!this->empty());
  return (*this)[this->size() - 1];
}

template <typename T>
typename Slice<T>::iterator::reference
Slice<T>::iterator::operator*() const noexcept {
  return *static_cast<T *>(this->pos);
}

template <typename T>
typename Slice<T>::iterator::pointer
Slice<T>::iterator::operator->() const noexcept {
  return static_cast<T *>(this->pos);
}

template <typename T>
typename Slice<T>::iterator::reference Slice<T>::iterator::operator[](
    typename Slice<T>::iterator::difference_type n) const noexcept {
  auto ptr = static_cast<char *>(this->pos) + this->stride * n;
  return *reinterpret_cast<T *>(ptr);
}

template <typename T>
typename Slice<T>::iterator &Slice<T>::iterator::operator++() noexcept {
  this->pos = static_cast<char *>(this->pos) + this->stride;
  return *this;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::iterator::operator++(int) noexcept {
  auto ret = iterator(*this);
  this->pos = static_cast<char *>(this->pos) + this->stride;
  return ret;
}

template <typename T>
typename Slice<T>::iterator &Slice<T>::iterator::operator--() noexcept {
  this->pos = static_cast<char *>(this->pos) - this->stride;
  return *this;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::iterator::operator--(int) noexcept {
  auto ret = iterator(*this);
  this->pos = static_cast<char *>(this->pos) - this->stride;
  return ret;
}

template <typename T>
typename Slice<T>::iterator &Slice<T>::iterator::operator+=(
    typename Slice<T>::iterator::difference_type n) noexcept {
  this->pos = static_cast<char *>(this->pos) + this->stride * n;
  return *this;
}

template <typename T>
typename Slice<T>::iterator &Slice<T>::iterator::operator-=(
    typename Slice<T>::iterator::difference_type n) noexcept {
  this->pos = static_cast<char *>(this->pos) - this->stride * n;
  return *this;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::iterator::operator+(
    typename Slice<T>::iterator::difference_type n) const noexcept {
  auto ret = iterator(*this);
  ret.pos = static_cast<char *>(this->pos) + this->stride * n;
  return ret;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::iterator::operator-(
    typename Slice<T>::iterator::difference_type n) const noexcept {
  auto ret = iterator(*this);
  ret.pos = static_cast<char *>(this->pos) - this->stride * n;
  return ret;
}

template <typename T>
typename Slice<T>::iterator::difference_type
Slice<T>::iterator::operator-(const iterator &other) const noexcept {
  auto diff = std::distance(static_cast<char *>(other.pos),
                            static_cast<char *>(this->pos));
  return diff / this->stride;
}

template <typename T>
bool Slice<T>::iterator::operator==(const iterator &other) const noexcept {
  return this->pos == other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator!=(const iterator &other) const noexcept {
  return this->pos != other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator<(const iterator &other) const noexcept {
  return this->pos < other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator<=(const iterator &other) const noexcept {
  return this->pos <= other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator>(const iterator &other) const noexcept {
  return this->pos > other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator>=(const iterator &other) const noexcept {
  return this->pos >= other.pos;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::begin() const noexcept {
  iterator it;
  it.pos = slicePtr(this);
  it.stride = size_of<T>();
  return it;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::end() const noexcept {
  iterator it = this->begin();
  it.pos = static_cast<char *>(it.pos) + it.stride * this->size();
  return it;
}

template <typename T>
void Slice<T>::swap(Slice &rhs) noexcept {
  std::swap(*this, rhs);
}
#endif // CXXBRIDGE1_RUST_SLICE

#ifndef CXXBRIDGE1_RUST_BOX
#define CXXBRIDGE1_RUST_BOX
template <typename T>
class Box final {
public:
  using element_type = T;
  using const_pointer =
      typename std::add_pointer<typename std::add_const<T>::type>::type;
  using pointer = typename std::add_pointer<T>::type;

  Box() = delete;
  Box(Box &&) noexcept;
  ~Box() noexcept;

  explicit Box(const T &);
  explicit Box(T &&);

  Box &operator=(Box &&) &noexcept;

  const T *operator->() const noexcept;
  const T &operator*() const noexcept;
  T *operator->() noexcept;
  T &operator*() noexcept;

  template <typename... Fields>
  static Box in_place(Fields &&...);

  void swap(Box &) noexcept;

  static Box from_raw(T *) noexcept;

  T *into_raw() noexcept;

  /* Deprecated */ using value_type = element_type;

private:
  class uninit;
  class allocation;
  Box(uninit) noexcept;
  void drop() noexcept;

  friend void swap(Box &lhs, Box &rhs) noexcept { lhs.swap(rhs); }

  T *ptr;
};

template <typename T>
class Box<T>::uninit {};

template <typename T>
class Box<T>::allocation {
  static T *alloc() noexcept;
  static void dealloc(T *) noexcept;

public:
  allocation() noexcept : ptr(alloc()) {}
  ~allocation() noexcept {
    if (this->ptr) {
      dealloc(this->ptr);
    }
  }
  T *ptr;
};

template <typename T>
Box<T>::Box(Box &&other) noexcept : ptr(other.ptr) {
  other.ptr = nullptr;
}

template <typename T>
Box<T>::Box(const T &val) {
  allocation alloc;
  ::new (alloc.ptr) T(val);
  this->ptr = alloc.ptr;
  alloc.ptr = nullptr;
}

template <typename T>
Box<T>::Box(T &&val) {
  allocation alloc;
  ::new (alloc.ptr) T(std::move(val));
  this->ptr = alloc.ptr;
  alloc.ptr = nullptr;
}

template <typename T>
Box<T>::~Box() noexcept {
  if (this->ptr) {
    this->drop();
  }
}

template <typename T>
Box<T> &Box<T>::operator=(Box &&other) &noexcept {
  if (this->ptr) {
    this->drop();
  }
  this->ptr = other.ptr;
  other.ptr = nullptr;
  return *this;
}

template <typename T>
const T *Box<T>::operator->() const noexcept {
  return this->ptr;
}

template <typename T>
const T &Box<T>::operator*() const noexcept {
  return *this->ptr;
}

template <typename T>
T *Box<T>::operator->() noexcept {
  return this->ptr;
}

template <typename T>
T &Box<T>::operator*() noexcept {
  return *this->ptr;
}

template <typename T>
template <typename... Fields>
Box<T> Box<T>::in_place(Fields &&...fields) {
  allocation alloc;
  auto ptr = alloc.ptr;
  ::new (ptr) T{std::forward<Fields>(fields)...};
  alloc.ptr = nullptr;
  return from_raw(ptr);
}

template <typename T>
void Box<T>::swap(Box &rhs) noexcept {
  using std::swap;
  swap(this->ptr, rhs.ptr);
}

template <typename T>
Box<T> Box<T>::from_raw(T *raw) noexcept {
  Box box = uninit{};
  box.ptr = raw;
  return box;
}

template <typename T>
T *Box<T>::into_raw() noexcept {
  T *raw = this->ptr;
  this->ptr = nullptr;
  return raw;
}

template <typename T>
Box<T>::Box(uninit) noexcept {}
#endif // CXXBRIDGE1_RUST_BOX

#ifndef CXXBRIDGE1_RUST_BITCOPY_T
#define CXXBRIDGE1_RUST_BITCOPY_T
struct unsafe_bitcopy_t final {
  explicit unsafe_bitcopy_t() = default;
};
#endif // CXXBRIDGE1_RUST_BITCOPY_T

#ifndef CXXBRIDGE1_RUST_VEC
#define CXXBRIDGE1_RUST_VEC
template <typename T>
class Vec final {
public:
  using value_type = T;

  Vec() noexcept;
  Vec(std::initializer_list<T>);
  Vec(const Vec &);
  Vec(Vec &&) noexcept;
  ~Vec() noexcept;

  Vec &operator=(Vec &&) &noexcept;
  Vec &operator=(const Vec &) &;

  std::size_t size() const noexcept;
  bool empty() const noexcept;
  const T *data() const noexcept;
  T *data() noexcept;
  std::size_t capacity() const noexcept;

  const T &operator[](std::size_t n) const noexcept;
  const T &at(std::size_t n) const;
  const T &front() const noexcept;
  const T &back() const noexcept;

  T &operator[](std::size_t n) noexcept;
  T &at(std::size_t n);
  T &front() noexcept;
  T &back() noexcept;

  void reserve(std::size_t new_cap);
  void push_back(const T &value);
  void push_back(T &&value);
  template <typename... Args>
  void emplace_back(Args &&...args);

  using iterator = typename Slice<T>::iterator;
  iterator begin() noexcept;
  iterator end() noexcept;

  using const_iterator = typename Slice<const T>::iterator;
  const_iterator begin() const noexcept;
  const_iterator end() const noexcept;
  const_iterator cbegin() const noexcept;
  const_iterator cend() const noexcept;

  void swap(Vec &) noexcept;

  Vec(unsafe_bitcopy_t, const Vec &) noexcept;

private:
  void reserve_total(std::size_t new_cap) noexcept;
  void set_len(std::size_t len) noexcept;
  void drop() noexcept;

  friend void swap(Vec &lhs, Vec &rhs) noexcept { lhs.swap(rhs); }

  std::array<std::uintptr_t, 3> repr;
};

template <typename T>
Vec<T>::Vec(std::initializer_list<T> init) : Vec{} {
  this->reserve_total(init.size());
  std::move(init.begin(), init.end(), std::back_inserter(*this));
}

template <typename T>
Vec<T>::Vec(const Vec &other) : Vec() {
  this->reserve_total(other.size());
  std::copy(other.begin(), other.end(), std::back_inserter(*this));
}

template <typename T>
Vec<T>::Vec(Vec &&other) noexcept : repr(other.repr) {
  new (&other) Vec();
}

template <typename T>
Vec<T>::~Vec() noexcept {
  this->drop();
}

template <typename T>
Vec<T> &Vec<T>::operator=(Vec &&other) &noexcept {
  this->drop();
  this->repr = other.repr;
  new (&other) Vec();
  return *this;
}

template <typename T>
Vec<T> &Vec<T>::operator=(const Vec &other) & {
  if (this != &other) {
    this->drop();
    new (this) Vec(other);
  }
  return *this;
}

template <typename T>
bool Vec<T>::empty() const noexcept {
  return this->size() == 0;
}

template <typename T>
T *Vec<T>::data() noexcept {
  return const_cast<T *>(const_cast<const Vec<T> *>(this)->data());
}

template <typename T>
const T &Vec<T>::operator[](std::size_t n) const noexcept {
  assert(n < this->size());
  auto data = reinterpret_cast<const char *>(this->data());
  return *reinterpret_cast<const T *>(data + n * size_of<T>());
}

template <typename T>
const T &Vec<T>::at(std::size_t n) const {
  if (n >= this->size()) {
    panic<std::out_of_range>("rust::Vec index out of range");
  }
  return (*this)[n];
}

template <typename T>
const T &Vec<T>::front() const noexcept {
  assert(!this->empty());
  return (*this)[0];
}

template <typename T>
const T &Vec<T>::back() const noexcept {
  assert(!this->empty());
  return (*this)[this->size() - 1];
}

template <typename T>
T &Vec<T>::operator[](std::size_t n) noexcept {
  assert(n < this->size());
  auto data = reinterpret_cast<char *>(this->data());
  return *reinterpret_cast<T *>(data + n * size_of<T>());
}

template <typename T>
T &Vec<T>::at(std::size_t n) {
  if (n >= this->size()) {
    panic<std::out_of_range>("rust::Vec index out of range");
  }
  return (*this)[n];
}

template <typename T>
T &Vec<T>::front() noexcept {
  assert(!this->empty());
  return (*this)[0];
}

template <typename T>
T &Vec<T>::back() noexcept {
  assert(!this->empty());
  return (*this)[this->size() - 1];
}

template <typename T>
void Vec<T>::reserve(std::size_t new_cap) {
  this->reserve_total(new_cap);
}

template <typename T>
void Vec<T>::push_back(const T &value) {
  this->emplace_back(value);
}

template <typename T>
void Vec<T>::push_back(T &&value) {
  this->emplace_back(std::move(value));
}

template <typename T>
template <typename... Args>
void Vec<T>::emplace_back(Args &&...args) {
  auto size = this->size();
  this->reserve_total(size + 1);
  ::new (reinterpret_cast<T *>(reinterpret_cast<char *>(this->data()) +
                               size * size_of<T>()))
      T(std::forward<Args>(args)...);
  this->set_len(size + 1);
}

template <typename T>
typename Vec<T>::iterator Vec<T>::begin() noexcept {
  return Slice<T>(this->data(), this->size()).begin();
}

template <typename T>
typename Vec<T>::iterator Vec<T>::end() noexcept {
  return Slice<T>(this->data(), this->size()).end();
}

template <typename T>
typename Vec<T>::const_iterator Vec<T>::begin() const noexcept {
  return this->cbegin();
}

template <typename T>
typename Vec<T>::const_iterator Vec<T>::end() const noexcept {
  return this->cend();
}

template <typename T>
typename Vec<T>::const_iterator Vec<T>::cbegin() const noexcept {
  return Slice<const T>(this->data(), this->size()).begin();
}

template <typename T>
typename Vec<T>::const_iterator Vec<T>::cend() const noexcept {
  return Slice<const T>(this->data(), this->size()).end();
}

template <typename T>
void Vec<T>::swap(Vec &rhs) noexcept {
  using std::swap;
  swap(this->repr, rhs.repr);
}

template <typename T>
Vec<T>::Vec(unsafe_bitcopy_t, const Vec &bits) noexcept : repr(bits.repr) {}
#endif // CXXBRIDGE1_RUST_VEC

#ifndef CXXBRIDGE1_RUST_FN
#define CXXBRIDGE1_RUST_FN
template <typename Signature>
class Fn;

template <typename Ret, typename... Args>
class Fn<Ret(Args...)> final {
public:
  Ret operator()(Args... args) const noexcept;
  Fn operator*() const noexcept;

private:
  Ret (*trampoline)(Args..., void *fn) noexcept;
  void *fn;
};

template <typename Ret, typename... Args>
Ret Fn<Ret(Args...)>::operator()(Args... args) const noexcept {
  return (*this->trampoline)(std::forward<Args>(args)..., this->fn);
}

template <typename Ret, typename... Args>
Fn<Ret(Args...)> Fn<Ret(Args...)>::operator*() const noexcept {
  return *this;
}
#endif // CXXBRIDGE1_RUST_FN

#ifndef CXXBRIDGE1_RUST_OPAQUE
#define CXXBRIDGE1_RUST_OPAQUE
class Opaque {
public:
  Opaque() = delete;
  Opaque(const Opaque &) = delete;
  ~Opaque() = delete;
};
#endif // CXXBRIDGE1_RUST_OPAQUE

#ifndef CXXBRIDGE1_IS_COMPLETE
#define CXXBRIDGE1_IS_COMPLETE
namespace detail {
namespace {
template <typename T, typename = std::size_t>
struct is_complete : std::false_type {};
template <typename T>
struct is_complete<T, decltype(sizeof(T))> : std::true_type {};
} // namespace
} // namespace detail
#endif // CXXBRIDGE1_IS_COMPLETE

#ifndef CXXBRIDGE1_LAYOUT
#define CXXBRIDGE1_LAYOUT
class layout {
  template <typename T>
  friend std::size_t size_of();
  template <typename T>
  friend std::size_t align_of();
  template <typename T>
  static typename std::enable_if<std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_size_of() {
    return T::layout::size();
  }
  template <typename T>
  static typename std::enable_if<!std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_size_of() {
    return sizeof(T);
  }
  template <typename T>
  static
      typename std::enable_if<detail::is_complete<T>::value, std::size_t>::type
      size_of() {
    return do_size_of<T>();
  }
  template <typename T>
  static typename std::enable_if<std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_align_of() {
    return T::layout::align();
  }
  template <typename T>
  static typename std::enable_if<!std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_align_of() {
    return alignof(T);
  }
  template <typename T>
  static
      typename std::enable_if<detail::is_complete<T>::value, std::size_t>::type
      align_of() {
    return do_align_of<T>();
  }
};

template <typename T>
std::size_t size_of() {
  return layout::size_of<T>();
}

template <typename T>
std::size_t align_of() {
  return layout::align_of<T>();
}
#endif // CXXBRIDGE1_LAYOUT

#ifndef CXXBRIDGE1_RELOCATABLE
#define CXXBRIDGE1_RELOCATABLE
namespace detail {
template <typename... Ts>
struct make_void {
  using type = void;
};

template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <typename Void, template <typename...> class, typename...>
struct detect : std::false_type {};
template <template <typename...> class T, typename... A>
struct detect<void_t<T<A...>>, T, A...> : std::true_type {};

template <template <typename...> class T, typename... A>
using is_detected = detect<void, T, A...>;

template <typename T>
using detect_IsRelocatable = typename T::IsRelocatable;

template <typename T>
struct get_IsRelocatable
    : std::is_same<typename T::IsRelocatable, std::true_type> {};
} // namespace detail

template <typename T>
struct IsRelocatable
    : std::conditional<
          detail::is_detected<detail::detect_IsRelocatable, T>::value,
          detail::get_IsRelocatable<T>,
          std::integral_constant<
              bool, std::is_trivially_move_constructible<T>::value &&
                        std::is_trivially_destructible<T>::value>>::type {};
#endif // CXXBRIDGE1_RELOCATABLE

template <typename T>
union ManuallyDrop {
  T value;
  ManuallyDrop(T &&value) : value(::std::move(value)) {}
  ~ManuallyDrop() {}
};

namespace {
template <bool> struct deleter_if {
  template <typename T> void operator()(T *) {}
};

template <> struct deleter_if<true> {
  template <typename T> void operator()(T *ptr) { ptr->~T(); }
};
} // namespace
} // namespace cxxbridge1
} // namespace rust

namespace brave_rewards {
  enum class TracingLevel : ::std::uint8_t;
  enum class RewardsResult : ::std::uint8_t;
  struct HttpRequest;
  struct HttpResponse;
  struct HttpRoundtripContext;
  struct WakeupContext;
  struct CppSDK;
  using SkusSdkContext = ::brave_rewards::SkusSdkContext;
  using SkusSdkFetcher = ::brave_rewards::SkusSdkFetcher;
  using RefreshOrderCallbackState = ::brave_rewards::RefreshOrderCallbackState;
  using FetchOrderCredentialsCallbackState = ::brave_rewards::FetchOrderCredentialsCallbackState;
  using PrepareCredentialsPresentationCallbackState = ::brave_rewards::PrepareCredentialsPresentationCallbackState;
  using CredentialSummaryCallbackState = ::brave_rewards::CredentialSummaryCallbackState;
}

namespace brave_rewards {
#ifndef CXXBRIDGE1_ENUM_brave_rewards$TracingLevel
#define CXXBRIDGE1_ENUM_brave_rewards$TracingLevel
enum class TracingLevel : ::std::uint8_t {
  // The "trace" level.
  //
  // Designates very low priority, often extremely verbose, information.
  Trace = 0,
  // The "debug" level.
  //
  // Designates lower priority information.
  Debug = 1,
  // The "info" level.
  //
  // Designates useful information.
  Info = 2,
  // The "warn" level.
  //
  // Designates hazardous situations.
  Warn = 3,
  // The "error" level.
  //
  // Designates very serious errors.
  Error = 4,
};
#endif // CXXBRIDGE1_ENUM_brave_rewards$TracingLevel

#ifndef CXXBRIDGE1_ENUM_brave_rewards$RewardsResult
#define CXXBRIDGE1_ENUM_brave_rewards$RewardsResult
enum class RewardsResult : ::std::uint8_t {
  Ok = 0,
  RequestFailed = 1,
  InternalServer = 2,
  BadRequest = 3,
  UnhandledStatus = 4,
  RetryLater = 5,
  NotFound = 6,
  SerializationFailed = 7,
  InvalidResponse = 8,
  InvalidProof = 9,
  QueryError = 10,
  OutOfCredentials = 11,
  StorageWriteFailed = 12,
  StorageReadFailed = 13,
  OrderUnpaid = 14,
  UnhandledVariant = 15,
  OrderLocationMismatch = 16,
  ItemCredentialsMissing = 17,
  ItemCredentialsExpired = 18,
  InvalidMerchantOrSku = 19,
  UnknownError = 20,
  BorrowFailed = 21,
};
#endif // CXXBRIDGE1_ENUM_brave_rewards$RewardsResult

#ifndef CXXBRIDGE1_STRUCT_brave_rewards$HttpRequest
#define CXXBRIDGE1_STRUCT_brave_rewards$HttpRequest
struct HttpRequest final {
  ::rust::String url;
  ::rust::String method;
  ::rust::Vec<::rust::String> headers;
  ::rust::Vec<::std::uint8_t> body;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_brave_rewards$HttpRequest

#ifndef CXXBRIDGE1_STRUCT_brave_rewards$HttpResponse
#define CXXBRIDGE1_STRUCT_brave_rewards$HttpResponse
struct HttpResponse final {
  ::brave_rewards::RewardsResult result;
  ::std::uint16_t return_code;
  const ::std::vector<::std::string> &headers;
  const ::std::vector<::std::uint8_t> &body;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_brave_rewards$HttpResponse

#ifndef CXXBRIDGE1_STRUCT_brave_rewards$HttpRoundtripContext
#define CXXBRIDGE1_STRUCT_brave_rewards$HttpRoundtripContext
struct HttpRoundtripContext final : public ::rust::Opaque {
  ~HttpRoundtripContext() = delete;

private:
  friend ::rust::layout;
  struct layout {
    static ::std::size_t size() noexcept;
    static ::std::size_t align() noexcept;
  };
};
#endif // CXXBRIDGE1_STRUCT_brave_rewards$HttpRoundtripContext

#ifndef CXXBRIDGE1_STRUCT_brave_rewards$WakeupContext
#define CXXBRIDGE1_STRUCT_brave_rewards$WakeupContext
struct WakeupContext final : public ::rust::Opaque {
  ~WakeupContext() = delete;

private:
  friend ::rust::layout;
  struct layout {
    static ::std::size_t size() noexcept;
    static ::std::size_t align() noexcept;
  };
};
#endif // CXXBRIDGE1_STRUCT_brave_rewards$WakeupContext

#ifndef CXXBRIDGE1_STRUCT_brave_rewards$CppSDK
#define CXXBRIDGE1_STRUCT_brave_rewards$CppSDK
struct CppSDK final : public ::rust::Opaque {
  void shutdown() const noexcept;
  void refresh_order(::brave_rewards::RefreshOrderCallback callback, ::std::unique_ptr<::brave_rewards::RefreshOrderCallbackState> callback_state, ::rust::String order_id) const noexcept;
  void fetch_order_credentials(::brave_rewards::FetchOrderCredentialsCallback callback, ::std::unique_ptr<::brave_rewards::FetchOrderCredentialsCallbackState> callback_state, ::rust::String order_id) const noexcept;
  void prepare_credentials_presentation(::brave_rewards::PrepareCredentialsPresentationCallback callback, ::std::unique_ptr<::brave_rewards::PrepareCredentialsPresentationCallbackState> callback_state, ::rust::String domain, ::rust::String path) const noexcept;
  void credential_summary(::brave_rewards::CredentialSummaryCallback callback, ::std::unique_ptr<::brave_rewards::CredentialSummaryCallbackState> callback_state, ::rust::String domain) const noexcept;
  ~CppSDK() = delete;

private:
  friend ::rust::layout;
  struct layout {
    static ::std::size_t size() noexcept;
    static ::std::size_t align() noexcept;
  };
};
#endif // CXXBRIDGE1_STRUCT_brave_rewards$CppSDK
} // namespace brave_rewards

static_assert(
    ::rust::IsRelocatable<::brave_rewards::RefreshOrderCallback>::value,
    "type brave_rewards::RefreshOrderCallback should be trivially move constructible and trivially destructible in C++ to be used as an argument of `refresh_order` in Rust");
static_assert(
    ::rust::IsRelocatable<::brave_rewards::FetchOrderCredentialsCallback>::value,
    "type brave_rewards::FetchOrderCredentialsCallback should be trivially move constructible and trivially destructible in C++ to be used as an argument of `fetch_order_credentials` in Rust");
static_assert(
    ::rust::IsRelocatable<::brave_rewards::PrepareCredentialsPresentationCallback>::value,
    "type brave_rewards::PrepareCredentialsPresentationCallback should be trivially move constructible and trivially destructible in C++ to be used as an argument of `prepare_credentials_presentation` in Rust");
static_assert(
    ::rust::IsRelocatable<::brave_rewards::CredentialSummaryCallback>::value,
    "type brave_rewards::CredentialSummaryCallback should be trivially move constructible and trivially destructible in C++ to be used as an argument of `credential_summary` in Rust");

namespace brave_rewards {
extern "C" {
::std::size_t brave_rewards$cxxbridge1$HttpRoundtripContext$operator$sizeof() noexcept;
::std::size_t brave_rewards$cxxbridge1$HttpRoundtripContext$operator$alignof() noexcept;
::std::size_t brave_rewards$cxxbridge1$WakeupContext$operator$sizeof() noexcept;
::std::size_t brave_rewards$cxxbridge1$WakeupContext$operator$alignof() noexcept;
::std::size_t brave_rewards$cxxbridge1$CppSDK$operator$sizeof() noexcept;
::std::size_t brave_rewards$cxxbridge1$CppSDK$operator$alignof() noexcept;

::brave_rewards::CppSDK *brave_rewards$cxxbridge1$initialize_sdk(::brave_rewards::SkusSdkContext *ctx, ::rust::String *env) noexcept;

void brave_rewards$cxxbridge1$CppSDK$shutdown(const ::brave_rewards::CppSDK &self) noexcept;

void brave_rewards$cxxbridge1$CppSDK$refresh_order(const ::brave_rewards::CppSDK &self, ::brave_rewards::RefreshOrderCallback *callback, ::brave_rewards::RefreshOrderCallbackState *callback_state, ::rust::String *order_id) noexcept;

void brave_rewards$cxxbridge1$CppSDK$fetch_order_credentials(const ::brave_rewards::CppSDK &self, ::brave_rewards::FetchOrderCredentialsCallback *callback, ::brave_rewards::FetchOrderCredentialsCallbackState *callback_state, ::rust::String *order_id) noexcept;

void brave_rewards$cxxbridge1$CppSDK$prepare_credentials_presentation(const ::brave_rewards::CppSDK &self, ::brave_rewards::PrepareCredentialsPresentationCallback *callback, ::brave_rewards::PrepareCredentialsPresentationCallbackState *callback_state, ::rust::String *domain, ::rust::String *path) noexcept;

void brave_rewards$cxxbridge1$CppSDK$credential_summary(const ::brave_rewards::CppSDK &self, ::brave_rewards::CredentialSummaryCallback *callback, ::brave_rewards::CredentialSummaryCallbackState *callback_state, ::rust::String *domain) noexcept;

void brave_rewards$cxxbridge1$shim_logMessage(::rust::Str file, ::std::uint32_t line, ::brave_rewards::TracingLevel level, ::rust::Str message) noexcept {
  void (*shim_logMessage$)(::rust::Str, ::std::uint32_t, ::brave_rewards::TracingLevel, ::rust::Str) = ::brave_rewards::shim_logMessage;
  shim_logMessage$(file, line, level, message);
}

::brave_rewards::SkusSdkFetcher *brave_rewards$cxxbridge1$shim_executeRequest(const ::brave_rewards::SkusSdkContext &ctx, const ::brave_rewards::HttpRequest &req, ::rust::Fn<void(::rust::Box<::brave_rewards::HttpRoundtripContext>, ::brave_rewards::HttpResponse)> done, ::brave_rewards::HttpRoundtripContext *rt_ctx) noexcept {
  ::std::unique_ptr<::brave_rewards::SkusSdkFetcher> (*shim_executeRequest$)(const ::brave_rewards::SkusSdkContext &, const ::brave_rewards::HttpRequest &, ::rust::Fn<void(::rust::Box<::brave_rewards::HttpRoundtripContext>, ::brave_rewards::HttpResponse)>, ::rust::Box<::brave_rewards::HttpRoundtripContext>) = ::brave_rewards::shim_executeRequest;
  return shim_executeRequest$(ctx, req, done, ::rust::Box<::brave_rewards::HttpRoundtripContext>::from_raw(rt_ctx)).release();
}

void brave_rewards$cxxbridge1$shim_executeRequest$done$1(::brave_rewards::HttpRoundtripContext *arg0, ::brave_rewards::HttpResponse resp, void *) noexcept;

void brave_rewards$cxxbridge1$shim_executeRequest$done$0(::rust::Box<::brave_rewards::HttpRoundtripContext> arg0, ::brave_rewards::HttpResponse resp, void *extern$) noexcept {
  brave_rewards$cxxbridge1$shim_executeRequest$done$1(arg0.into_raw(), resp, extern$);
}

void brave_rewards$cxxbridge1$shim_scheduleWakeup(::std::uint64_t delay_ms, ::rust::Fn<void(::rust::Box<::brave_rewards::WakeupContext>)> done, ::brave_rewards::WakeupContext *ctx) noexcept {
  void (*shim_scheduleWakeup$)(::std::uint64_t, ::rust::Fn<void(::rust::Box<::brave_rewards::WakeupContext>)>, ::rust::Box<::brave_rewards::WakeupContext>) = ::brave_rewards::shim_scheduleWakeup;
  shim_scheduleWakeup$(delay_ms, done, ::rust::Box<::brave_rewards::WakeupContext>::from_raw(ctx));
}

void brave_rewards$cxxbridge1$shim_scheduleWakeup$done$1(::brave_rewards::WakeupContext *arg0, void *) noexcept;

void brave_rewards$cxxbridge1$shim_scheduleWakeup$done$0(::rust::Box<::brave_rewards::WakeupContext> arg0, void *extern$) noexcept {
  brave_rewards$cxxbridge1$shim_scheduleWakeup$done$1(arg0.into_raw(), extern$);
}

void brave_rewards$cxxbridge1$shim_purge(::brave_rewards::SkusSdkContext &ctx) noexcept {
  void (*shim_purge$)(::brave_rewards::SkusSdkContext &) = ::brave_rewards::shim_purge;
  shim_purge$(ctx);
}

void brave_rewards$cxxbridge1$shim_set(::brave_rewards::SkusSdkContext &ctx, ::rust::Str key, ::rust::Str value) noexcept {
  void (*shim_set$)(::brave_rewards::SkusSdkContext &, ::rust::Str, ::rust::Str) = ::brave_rewards::shim_set;
  shim_set$(ctx, key, value);
}

void brave_rewards$cxxbridge1$shim_get(::brave_rewards::SkusSdkContext &ctx, ::rust::Str key, ::rust::String *return$) noexcept {
  ::rust::String (*shim_get$)(::brave_rewards::SkusSdkContext &, ::rust::Str) = ::brave_rewards::shim_get;
  new (return$) ::rust::String(shim_get$(ctx, key));
}
} // extern "C"

::std::size_t HttpRoundtripContext::layout::size() noexcept {
  return brave_rewards$cxxbridge1$HttpRoundtripContext$operator$sizeof();
}

::std::size_t HttpRoundtripContext::layout::align() noexcept {
  return brave_rewards$cxxbridge1$HttpRoundtripContext$operator$alignof();
}

::std::size_t WakeupContext::layout::size() noexcept {
  return brave_rewards$cxxbridge1$WakeupContext$operator$sizeof();
}

::std::size_t WakeupContext::layout::align() noexcept {
  return brave_rewards$cxxbridge1$WakeupContext$operator$alignof();
}

::std::size_t CppSDK::layout::size() noexcept {
  return brave_rewards$cxxbridge1$CppSDK$operator$sizeof();
}

::std::size_t CppSDK::layout::align() noexcept {
  return brave_rewards$cxxbridge1$CppSDK$operator$alignof();
}

::rust::Box<::brave_rewards::CppSDK> initialize_sdk(::std::unique_ptr<::brave_rewards::SkusSdkContext> ctx, ::rust::String env) noexcept {
  return ::rust::Box<::brave_rewards::CppSDK>::from_raw(brave_rewards$cxxbridge1$initialize_sdk(ctx.release(), &env));
}

void CppSDK::shutdown() const noexcept {
  brave_rewards$cxxbridge1$CppSDK$shutdown(*this);
}

void CppSDK::refresh_order(::brave_rewards::RefreshOrderCallback callback, ::std::unique_ptr<::brave_rewards::RefreshOrderCallbackState> callback_state, ::rust::String order_id) const noexcept {
  ::rust::ManuallyDrop<::brave_rewards::RefreshOrderCallback> callback$(::std::move(callback));
  brave_rewards$cxxbridge1$CppSDK$refresh_order(*this, &callback$.value, callback_state.release(), &order_id);
}

void CppSDK::fetch_order_credentials(::brave_rewards::FetchOrderCredentialsCallback callback, ::std::unique_ptr<::brave_rewards::FetchOrderCredentialsCallbackState> callback_state, ::rust::String order_id) const noexcept {
  ::rust::ManuallyDrop<::brave_rewards::FetchOrderCredentialsCallback> callback$(::std::move(callback));
  brave_rewards$cxxbridge1$CppSDK$fetch_order_credentials(*this, &callback$.value, callback_state.release(), &order_id);
}

void CppSDK::prepare_credentials_presentation(::brave_rewards::PrepareCredentialsPresentationCallback callback, ::std::unique_ptr<::brave_rewards::PrepareCredentialsPresentationCallbackState> callback_state, ::rust::String domain, ::rust::String path) const noexcept {
  ::rust::ManuallyDrop<::brave_rewards::PrepareCredentialsPresentationCallback> callback$(::std::move(callback));
  brave_rewards$cxxbridge1$CppSDK$prepare_credentials_presentation(*this, &callback$.value, callback_state.release(), &domain, &path);
}

void CppSDK::credential_summary(::brave_rewards::CredentialSummaryCallback callback, ::std::unique_ptr<::brave_rewards::CredentialSummaryCallbackState> callback_state, ::rust::String domain) const noexcept {
  ::rust::ManuallyDrop<::brave_rewards::CredentialSummaryCallback> callback$(::std::move(callback));
  brave_rewards$cxxbridge1$CppSDK$credential_summary(*this, &callback$.value, callback_state.release(), &domain);
}
} // namespace brave_rewards

extern "C" {
static_assert(::rust::detail::is_complete<::brave_rewards::SkusSdkContext>::value, "definition of SkusSdkContext is required");
static_assert(sizeof(::std::unique_ptr<::brave_rewards::SkusSdkContext>) == sizeof(void *), "");
static_assert(alignof(::std::unique_ptr<::brave_rewards::SkusSdkContext>) == alignof(void *), "");
void cxxbridge1$unique_ptr$brave_rewards$SkusSdkContext$null(::std::unique_ptr<::brave_rewards::SkusSdkContext> *ptr) noexcept {
  ::new (ptr) ::std::unique_ptr<::brave_rewards::SkusSdkContext>();
}
void cxxbridge1$unique_ptr$brave_rewards$SkusSdkContext$raw(::std::unique_ptr<::brave_rewards::SkusSdkContext> *ptr, ::brave_rewards::SkusSdkContext *raw) noexcept {
  ::new (ptr) ::std::unique_ptr<::brave_rewards::SkusSdkContext>(raw);
}
const ::brave_rewards::SkusSdkContext *cxxbridge1$unique_ptr$brave_rewards$SkusSdkContext$get(const ::std::unique_ptr<::brave_rewards::SkusSdkContext>& ptr) noexcept {
  return ptr.get();
}
::brave_rewards::SkusSdkContext *cxxbridge1$unique_ptr$brave_rewards$SkusSdkContext$release(::std::unique_ptr<::brave_rewards::SkusSdkContext>& ptr) noexcept {
  return ptr.release();
}
void cxxbridge1$unique_ptr$brave_rewards$SkusSdkContext$drop(::std::unique_ptr<::brave_rewards::SkusSdkContext> *ptr) noexcept {
  ::rust::deleter_if<::rust::detail::is_complete<::brave_rewards::SkusSdkContext>::value>{}(ptr);
}

::brave_rewards::CppSDK *cxxbridge1$box$brave_rewards$CppSDK$alloc() noexcept;
void cxxbridge1$box$brave_rewards$CppSDK$dealloc(::brave_rewards::CppSDK *) noexcept;
void cxxbridge1$box$brave_rewards$CppSDK$drop(::rust::Box<::brave_rewards::CppSDK> *ptr) noexcept;

static_assert(::rust::detail::is_complete<::brave_rewards::RefreshOrderCallbackState>::value, "definition of RefreshOrderCallbackState is required");
static_assert(sizeof(::std::unique_ptr<::brave_rewards::RefreshOrderCallbackState>) == sizeof(void *), "");
static_assert(alignof(::std::unique_ptr<::brave_rewards::RefreshOrderCallbackState>) == alignof(void *), "");
void cxxbridge1$unique_ptr$brave_rewards$RefreshOrderCallbackState$null(::std::unique_ptr<::brave_rewards::RefreshOrderCallbackState> *ptr) noexcept {
  ::new (ptr) ::std::unique_ptr<::brave_rewards::RefreshOrderCallbackState>();
}
void cxxbridge1$unique_ptr$brave_rewards$RefreshOrderCallbackState$raw(::std::unique_ptr<::brave_rewards::RefreshOrderCallbackState> *ptr, ::brave_rewards::RefreshOrderCallbackState *raw) noexcept {
  ::new (ptr) ::std::unique_ptr<::brave_rewards::RefreshOrderCallbackState>(raw);
}
const ::brave_rewards::RefreshOrderCallbackState *cxxbridge1$unique_ptr$brave_rewards$RefreshOrderCallbackState$get(const ::std::unique_ptr<::brave_rewards::RefreshOrderCallbackState>& ptr) noexcept {
  return ptr.get();
}
::brave_rewards::RefreshOrderCallbackState *cxxbridge1$unique_ptr$brave_rewards$RefreshOrderCallbackState$release(::std::unique_ptr<::brave_rewards::RefreshOrderCallbackState>& ptr) noexcept {
  return ptr.release();
}
void cxxbridge1$unique_ptr$brave_rewards$RefreshOrderCallbackState$drop(::std::unique_ptr<::brave_rewards::RefreshOrderCallbackState> *ptr) noexcept {
  ::rust::deleter_if<::rust::detail::is_complete<::brave_rewards::RefreshOrderCallbackState>::value>{}(ptr);
}

static_assert(::rust::detail::is_complete<::brave_rewards::FetchOrderCredentialsCallbackState>::value, "definition of FetchOrderCredentialsCallbackState is required");
static_assert(sizeof(::std::unique_ptr<::brave_rewards::FetchOrderCredentialsCallbackState>) == sizeof(void *), "");
static_assert(alignof(::std::unique_ptr<::brave_rewards::FetchOrderCredentialsCallbackState>) == alignof(void *), "");
void cxxbridge1$unique_ptr$brave_rewards$FetchOrderCredentialsCallbackState$null(::std::unique_ptr<::brave_rewards::FetchOrderCredentialsCallbackState> *ptr) noexcept {
  ::new (ptr) ::std::unique_ptr<::brave_rewards::FetchOrderCredentialsCallbackState>();
}
void cxxbridge1$unique_ptr$brave_rewards$FetchOrderCredentialsCallbackState$raw(::std::unique_ptr<::brave_rewards::FetchOrderCredentialsCallbackState> *ptr, ::brave_rewards::FetchOrderCredentialsCallbackState *raw) noexcept {
  ::new (ptr) ::std::unique_ptr<::brave_rewards::FetchOrderCredentialsCallbackState>(raw);
}
const ::brave_rewards::FetchOrderCredentialsCallbackState *cxxbridge1$unique_ptr$brave_rewards$FetchOrderCredentialsCallbackState$get(const ::std::unique_ptr<::brave_rewards::FetchOrderCredentialsCallbackState>& ptr) noexcept {
  return ptr.get();
}
::brave_rewards::FetchOrderCredentialsCallbackState *cxxbridge1$unique_ptr$brave_rewards$FetchOrderCredentialsCallbackState$release(::std::unique_ptr<::brave_rewards::FetchOrderCredentialsCallbackState>& ptr) noexcept {
  return ptr.release();
}
void cxxbridge1$unique_ptr$brave_rewards$FetchOrderCredentialsCallbackState$drop(::std::unique_ptr<::brave_rewards::FetchOrderCredentialsCallbackState> *ptr) noexcept {
  ::rust::deleter_if<::rust::detail::is_complete<::brave_rewards::FetchOrderCredentialsCallbackState>::value>{}(ptr);
}

static_assert(::rust::detail::is_complete<::brave_rewards::PrepareCredentialsPresentationCallbackState>::value, "definition of PrepareCredentialsPresentationCallbackState is required");
static_assert(sizeof(::std::unique_ptr<::brave_rewards::PrepareCredentialsPresentationCallbackState>) == sizeof(void *), "");
static_assert(alignof(::std::unique_ptr<::brave_rewards::PrepareCredentialsPresentationCallbackState>) == alignof(void *), "");
void cxxbridge1$unique_ptr$brave_rewards$PrepareCredentialsPresentationCallbackState$null(::std::unique_ptr<::brave_rewards::PrepareCredentialsPresentationCallbackState> *ptr) noexcept {
  ::new (ptr) ::std::unique_ptr<::brave_rewards::PrepareCredentialsPresentationCallbackState>();
}
void cxxbridge1$unique_ptr$brave_rewards$PrepareCredentialsPresentationCallbackState$raw(::std::unique_ptr<::brave_rewards::PrepareCredentialsPresentationCallbackState> *ptr, ::brave_rewards::PrepareCredentialsPresentationCallbackState *raw) noexcept {
  ::new (ptr) ::std::unique_ptr<::brave_rewards::PrepareCredentialsPresentationCallbackState>(raw);
}
const ::brave_rewards::PrepareCredentialsPresentationCallbackState *cxxbridge1$unique_ptr$brave_rewards$PrepareCredentialsPresentationCallbackState$get(const ::std::unique_ptr<::brave_rewards::PrepareCredentialsPresentationCallbackState>& ptr) noexcept {
  return ptr.get();
}
::brave_rewards::PrepareCredentialsPresentationCallbackState *cxxbridge1$unique_ptr$brave_rewards$PrepareCredentialsPresentationCallbackState$release(::std::unique_ptr<::brave_rewards::PrepareCredentialsPresentationCallbackState>& ptr) noexcept {
  return ptr.release();
}
void cxxbridge1$unique_ptr$brave_rewards$PrepareCredentialsPresentationCallbackState$drop(::std::unique_ptr<::brave_rewards::PrepareCredentialsPresentationCallbackState> *ptr) noexcept {
  ::rust::deleter_if<::rust::detail::is_complete<::brave_rewards::PrepareCredentialsPresentationCallbackState>::value>{}(ptr);
}

static_assert(::rust::detail::is_complete<::brave_rewards::CredentialSummaryCallbackState>::value, "definition of CredentialSummaryCallbackState is required");
static_assert(sizeof(::std::unique_ptr<::brave_rewards::CredentialSummaryCallbackState>) == sizeof(void *), "");
static_assert(alignof(::std::unique_ptr<::brave_rewards::CredentialSummaryCallbackState>) == alignof(void *), "");
void cxxbridge1$unique_ptr$brave_rewards$CredentialSummaryCallbackState$null(::std::unique_ptr<::brave_rewards::CredentialSummaryCallbackState> *ptr) noexcept {
  ::new (ptr) ::std::unique_ptr<::brave_rewards::CredentialSummaryCallbackState>();
}
void cxxbridge1$unique_ptr$brave_rewards$CredentialSummaryCallbackState$raw(::std::unique_ptr<::brave_rewards::CredentialSummaryCallbackState> *ptr, ::brave_rewards::CredentialSummaryCallbackState *raw) noexcept {
  ::new (ptr) ::std::unique_ptr<::brave_rewards::CredentialSummaryCallbackState>(raw);
}
const ::brave_rewards::CredentialSummaryCallbackState *cxxbridge1$unique_ptr$brave_rewards$CredentialSummaryCallbackState$get(const ::std::unique_ptr<::brave_rewards::CredentialSummaryCallbackState>& ptr) noexcept {
  return ptr.get();
}
::brave_rewards::CredentialSummaryCallbackState *cxxbridge1$unique_ptr$brave_rewards$CredentialSummaryCallbackState$release(::std::unique_ptr<::brave_rewards::CredentialSummaryCallbackState>& ptr) noexcept {
  return ptr.release();
}
void cxxbridge1$unique_ptr$brave_rewards$CredentialSummaryCallbackState$drop(::std::unique_ptr<::brave_rewards::CredentialSummaryCallbackState> *ptr) noexcept {
  ::rust::deleter_if<::rust::detail::is_complete<::brave_rewards::CredentialSummaryCallbackState>::value>{}(ptr);
}

::brave_rewards::HttpRoundtripContext *cxxbridge1$box$brave_rewards$HttpRoundtripContext$alloc() noexcept;
void cxxbridge1$box$brave_rewards$HttpRoundtripContext$dealloc(::brave_rewards::HttpRoundtripContext *) noexcept;
void cxxbridge1$box$brave_rewards$HttpRoundtripContext$drop(::rust::Box<::brave_rewards::HttpRoundtripContext> *ptr) noexcept;

static_assert(::rust::detail::is_complete<::brave_rewards::SkusSdkFetcher>::value, "definition of SkusSdkFetcher is required");
static_assert(sizeof(::std::unique_ptr<::brave_rewards::SkusSdkFetcher>) == sizeof(void *), "");
static_assert(alignof(::std::unique_ptr<::brave_rewards::SkusSdkFetcher>) == alignof(void *), "");
void cxxbridge1$unique_ptr$brave_rewards$SkusSdkFetcher$null(::std::unique_ptr<::brave_rewards::SkusSdkFetcher> *ptr) noexcept {
  ::new (ptr) ::std::unique_ptr<::brave_rewards::SkusSdkFetcher>();
}
void cxxbridge1$unique_ptr$brave_rewards$SkusSdkFetcher$raw(::std::unique_ptr<::brave_rewards::SkusSdkFetcher> *ptr, ::brave_rewards::SkusSdkFetcher *raw) noexcept {
  ::new (ptr) ::std::unique_ptr<::brave_rewards::SkusSdkFetcher>(raw);
}
const ::brave_rewards::SkusSdkFetcher *cxxbridge1$unique_ptr$brave_rewards$SkusSdkFetcher$get(const ::std::unique_ptr<::brave_rewards::SkusSdkFetcher>& ptr) noexcept {
  return ptr.get();
}
::brave_rewards::SkusSdkFetcher *cxxbridge1$unique_ptr$brave_rewards$SkusSdkFetcher$release(::std::unique_ptr<::brave_rewards::SkusSdkFetcher>& ptr) noexcept {
  return ptr.release();
}
void cxxbridge1$unique_ptr$brave_rewards$SkusSdkFetcher$drop(::std::unique_ptr<::brave_rewards::SkusSdkFetcher> *ptr) noexcept {
  ::rust::deleter_if<::rust::detail::is_complete<::brave_rewards::SkusSdkFetcher>::value>{}(ptr);
}

::brave_rewards::WakeupContext *cxxbridge1$box$brave_rewards$WakeupContext$alloc() noexcept;
void cxxbridge1$box$brave_rewards$WakeupContext$dealloc(::brave_rewards::WakeupContext *) noexcept;
void cxxbridge1$box$brave_rewards$WakeupContext$drop(::rust::Box<::brave_rewards::WakeupContext> *ptr) noexcept;
} // extern "C"

namespace rust {
inline namespace cxxbridge1 {
template <>
::brave_rewards::CppSDK *Box<::brave_rewards::CppSDK>::allocation::alloc() noexcept {
  return cxxbridge1$box$brave_rewards$CppSDK$alloc();
}
template <>
void Box<::brave_rewards::CppSDK>::allocation::dealloc(::brave_rewards::CppSDK *ptr) noexcept {
  cxxbridge1$box$brave_rewards$CppSDK$dealloc(ptr);
}
template <>
void Box<::brave_rewards::CppSDK>::drop() noexcept {
  cxxbridge1$box$brave_rewards$CppSDK$drop(this);
}
template <>
::brave_rewards::HttpRoundtripContext *Box<::brave_rewards::HttpRoundtripContext>::allocation::alloc() noexcept {
  return cxxbridge1$box$brave_rewards$HttpRoundtripContext$alloc();
}
template <>
void Box<::brave_rewards::HttpRoundtripContext>::allocation::dealloc(::brave_rewards::HttpRoundtripContext *ptr) noexcept {
  cxxbridge1$box$brave_rewards$HttpRoundtripContext$dealloc(ptr);
}
template <>
void Box<::brave_rewards::HttpRoundtripContext>::drop() noexcept {
  cxxbridge1$box$brave_rewards$HttpRoundtripContext$drop(this);
}
template <>
::brave_rewards::WakeupContext *Box<::brave_rewards::WakeupContext>::allocation::alloc() noexcept {
  return cxxbridge1$box$brave_rewards$WakeupContext$alloc();
}
template <>
void Box<::brave_rewards::WakeupContext>::allocation::dealloc(::brave_rewards::WakeupContext *ptr) noexcept {
  cxxbridge1$box$brave_rewards$WakeupContext$dealloc(ptr);
}
template <>
void Box<::brave_rewards::WakeupContext>::drop() noexcept {
  cxxbridge1$box$brave_rewards$WakeupContext$drop(this);
}
} // namespace cxxbridge1
} // namespace rust
