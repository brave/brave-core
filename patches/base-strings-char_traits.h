diff --git a/base/strings/char_traits.h b/base/strings/char_traits.h
index 67798b76cff2a6a3e38ad1329b73dc579df8096f..5cdc2cc147f3ef947d278be7b460273d3b857a58 100644
--- a/base/strings/char_traits.h
+++ b/base/strings/char_traits.h
@@ -13,6 +13,19 @@
 
 namespace base {
 
+namespace internal {
+
+template <typename T>
+constexpr const T* ConstexprFind(const T* s, size_t n, T c) {
+  for (; n; --n, ++s) {
+    if (std::char_traits<T>::eq(*s, c))
+      return s;
+  }
+  return nullptr;
+}
+
+}  // namespace internal
+
 // constexpr version of http://en.cppreference.com/w/cpp/string/char_traits.
 // This currently just implements the bits needed to support a (mostly)
 // constexpr StringPiece.
@@ -62,11 +75,7 @@ constexpr size_t CharTraits<T>::length(const T* s) noexcept {
 
 template <typename T>
 constexpr const T* CharTraits<T>::find(const T* s, size_t n, T c) {
-  for (; n; --n, ++s) {
-    if (std::char_traits<T>::eq(*s, c))
-      return s;
-  }
-  return nullptr;
+  return internal::ConstexprFind(s, n, c);
 }
 
 // char and wchar_t specialization of CharTraits that can use clang's constexpr
@@ -102,7 +111,9 @@ struct CharTraits<wchar_t> {
   }
 
   static constexpr const wchar_t* find(const wchar_t* s, size_t n, wchar_t c) {
-    return __builtin_wmemchr(s, c, n);
+    // Note: Due to https://bugs.llvm.org/show_bug.cgi?id=41226 we are not
+    // calling `__builtin_wmemchr` here.
+    return internal::ConstexprFind(s, n, c);
   }
 };
 #endif
