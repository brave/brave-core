use crate::sys::{jboolean, jbyte, jchar, jdouble, jfloat, jint, jlong, jshort};

pub(crate) mod type_array_sealed {
    use jni_sys::jsize;

    use crate::sys::{jarray, jboolean, jbyte, jchar, jdouble, jfloat, jint, jlong, jshort};
    use crate::{env::Env, errors::*};
    use std::ptr::NonNull;

    /// Trait to define type array access/release
    ///
    /// # Safety
    ///
    /// The methods of this trait must uphold the invariants described in [`Env::unsafe_clone`] when
    /// using the provided [`Env`].
    ///
    /// The `get` method must return a valid pointer to the beginning of the JNI array.
    ///
    /// The `release` method must not invalidate the `ptr` if the `mode` is [`sys::JNI_COMMIT`].
    pub unsafe trait TypeArraySealed: Copy + Send + Sync {
        /// Creates a new array of this type with the given length.
        unsafe fn new_array(env: &mut Env, length: jsize) -> Result<jarray>;

        /// getter
        ///
        /// # Safety
        ///
        /// `array` must be a valid pointer to an `Array` object, or `null`
        ///
        /// The caller is responsible for passing the returned pointer to [`release`], along
        /// with the same `env` and `array` reference (which needs to still be valid)
        unsafe fn get_elements(
            env: &Env,
            array: jarray,
            is_copy: &mut jboolean,
        ) -> Result<*mut Self>;

        /// releaser
        ///
        /// # Safety
        ///
        /// `array` must be a valid pointer to an `Array` object, or `null`
        ///
        /// `ptr` must have been previously returned by the `get` function.
        ///
        /// If `mode` is not [`sys::JNI_COMMIT`], `ptr` must not be used again after calling this
        /// function.
        unsafe fn release_elements(
            env: &Env,
            array: jarray,
            ptr: NonNull<Self>,
            mode: i32,
        ) -> Result<()>;

        /// Copy elements of the array from the `start` index to the `buf` slice. The number of
        /// copied elements is equal to the `buf` length.
        ///
        /// # Errors
        ///
        /// If `start` is negative _or_ `start + buf.len()` is greater than [`JPrimitiveArray::len`]
        /// then no elements are copied, an `ArrayIndexOutOfBoundsException` is thrown, and `Err` is
        /// returned.
        ///
        /// # Safety
        ///
        /// `array` must be a valid pointer to a primitive JNI array object, matching the type of
        /// `Self`, or `null`.
        unsafe fn get_region(
            env: &Env,
            array: jarray,
            start: jsize,
            buf: &mut [Self],
        ) -> Result<()>;

        /// Copy the contents of the `buf` slice to the java byte array at the
        /// `start` index.
        ///
        /// # Safety
        ///
        /// `array` must be a valid pointer to a primitive JNI array object, matching the type of
        /// `Self`, or `null`.
        unsafe fn set_region(env: &Env, array: jarray, start: jsize, buf: &[Self]) -> Result<()>;
    }

    // TypeArray builder
    macro_rules! type_array {
        ( $jni_type:ty, $jni_type_name:ident, $new_array:ident, $get_elements:ident, $release_elements:ident, $get_region:ident, $set_region:ident) => {
            /// $jni_type array access/release impl
            unsafe impl TypeArraySealed for $jni_type {
                /// Create new Java $jni_type array
                unsafe fn new_array(env: &mut Env, length: jsize) -> Result<jarray> {
                    let raw_array = unsafe {
                        jni_call_with_catch_and_null_check!(
                            catch |env| {
                                crate::exceptions::JOutOfMemoryError =>
                                    Err(Error::JniCall(JniError::NoMemory)),
                                else => Err(Error::JniCall(JniError::Unknown)),
                            },
                            env, v1_1, $new_array, length)?
                    };
                    Ok(raw_array)
                }

                /// Get Java $jni_type array
                unsafe fn get_elements(
                    env: &Env,
                    array: jarray,
                    is_copy: &mut jboolean,
                ) -> Result<*mut Self> {
                    // There are no documented exceptions for Get<Primitive>ArrayElements() but
                    // they may return `NULL`.
                    let ptr = unsafe {
                        jni_call_with_catch_and_null_check!(
                            catch |env| {
                                crate::exceptions::JOutOfMemoryError =>
                                    Err(Error::JniCall(JniError::NoMemory)),
                                else => Err(Error::JniCall(JniError::Unknown)),
                            },
                            env, v1_1, $get_elements, array, is_copy)?
                    };
                    Ok(ptr as _)
                }

                /// Release Java $jni_type array
                unsafe fn release_elements(
                    env: &Env,
                    array: jarray,
                    ptr: NonNull<Self>,
                    mode: i32,
                ) -> Result<()> {
                    // Release<Primitive>ArrayElements() is safe to call while there is a
                    // pending exception and there are no documented exceptions for
                    // Release<Primitive>ArrayElements()
                    unsafe {
                        ex_safe_jni_call_no_post_check_ex!(
                            env,
                            v1_1,
                            $release_elements,
                            array,
                            ptr.as_ptr(),
                            mode as i32
                        )
                    };
                    Ok(())
                }

                unsafe fn get_region(
                    env: &Env,
                    array: jarray,
                    start: jsize,
                    buf: &mut [Self],
                ) -> Result<()> {
                    let array = null_check!(array, "get_*_array_region array argument")?;
                    unsafe {
                        Ok(jni_call_with_catch!(
                            catch |env| {
                                crate::exceptions::JArrayIndexOutOfBoundsException =>
                                    Err(Error::IndexOutOfBounds),
                                else => Err(Error::JniCall(JniError::Unknown)),
                            },
                            env,
                            v1_1,
                            $get_region,
                            array,
                            start,
                            buf.len() as jsize,
                            buf.as_mut_ptr()
                        )?)
                    }
                }

                unsafe fn set_region(
                    env: &Env,
                    array: jarray,
                    start: jsize,
                    buf: &[Self],
                ) -> Result<()> {
                    let array = null_check!(array, "set_*_array_region array argument")?;
                    unsafe {
                        Ok(jni_call_with_catch!(
                            catch |env| {
                                crate::exceptions::JArrayIndexOutOfBoundsException =>
                                    Err(Error::IndexOutOfBounds),
                                else => Err(Error::JniCall(JniError::Unknown)),
                            },
                            env,
                            v1_1,
                            $set_region,
                            array,
                            start,
                            buf.len() as jsize,
                            buf.as_ptr()
                        )?)
                    }
                }
            }
        };
    }

    type_array!(
        jint,
        Int,
        NewIntArray,
        GetIntArrayElements,
        ReleaseIntArrayElements,
        GetIntArrayRegion,
        SetIntArrayRegion
    );
    type_array!(
        jlong,
        Long,
        NewLongArray,
        GetLongArrayElements,
        ReleaseLongArrayElements,
        GetLongArrayRegion,
        SetLongArrayRegion
    );
    type_array!(
        jbyte,
        Byte,
        NewByteArray,
        GetByteArrayElements,
        ReleaseByteArrayElements,
        GetByteArrayRegion,
        SetByteArrayRegion
    );
    type_array!(
        jboolean,
        Boolean,
        NewBooleanArray,
        GetBooleanArrayElements,
        ReleaseBooleanArrayElements,
        GetBooleanArrayRegion,
        SetBooleanArrayRegion
    );
    type_array!(
        jchar,
        Char,
        NewCharArray,
        GetCharArrayElements,
        ReleaseCharArrayElements,
        GetCharArrayRegion,
        SetCharArrayRegion
    );
    type_array!(
        jshort,
        Short,
        NewShortArray,
        GetShortArrayElements,
        ReleaseShortArrayElements,
        GetShortArrayRegion,
        SetShortArrayRegion
    );
    type_array!(
        jfloat,
        Float,
        NewFloatArray,
        GetFloatArrayElements,
        ReleaseFloatArrayElements,
        GetFloatArrayRegion,
        SetFloatArrayRegion
    );
    type_array!(
        jdouble,
        Double,
        NewDoubleArray,
        GetDoubleArrayElements,
        ReleaseDoubleArrayElements,
        GetDoubleArrayRegion,
        SetDoubleArrayRegion
    );
}

/// A sealed trait to define type array access/release for primitive JNI types
pub trait TypeArray: type_array_sealed::TypeArraySealed {}

impl TypeArray for jint {}
impl TypeArray for jlong {}
impl TypeArray for jbyte {}
impl TypeArray for jboolean {}
impl TypeArray for jchar {}
impl TypeArray for jshort {}
impl TypeArray for jfloat {}
impl TypeArray for jdouble {}
