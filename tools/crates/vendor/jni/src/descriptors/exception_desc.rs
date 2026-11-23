use crate::{
    descriptors::Desc,
    env::Env,
    errors::*,
    jni_str,
    objects::{Auto, IntoAuto as _, JClass, JObject, JString, JThrowable, JValue},
    signature::{JavaType, MethodSignature, Primitive},
    strings::{JNIStr, JNIString},
};

const DEFAULT_EXCEPTION_CLASS: &JNIStr = jni_str!("java/lang/RuntimeException");

unsafe impl<'local, 'other_local, C, M> Desc<'local, JThrowable<'local>> for (C, M)
where
    C: Desc<'local, JClass<'other_local>>,
    M: AsRef<JNIStr>,
{
    type Output = Auto<'local, JThrowable<'local>>;

    fn lookup(self, env: &mut Env<'local>) -> Result<Self::Output> {
        let jmsg = JString::from_jni_str(env, self.1.as_ref())?.auto();
        let ctor_args = &[JavaType::Object];
        // Safety: We are sure the arguments and return type are consistent with the signature string.
        let ctor_sig = unsafe {
            MethodSignature::from_raw_parts(
                jni_str!("(Ljava/lang/String;)V"),
                ctor_args,
                JavaType::Primitive(Primitive::Void),
            )
        };
        let obj: JObject = env.new_object(self.0, &ctor_sig, &[JValue::from(&jmsg)])?;
        let throwable = env.cast_local::<JThrowable>(obj)?;
        Ok(throwable.auto())
    }
}

unsafe impl<'local> Desc<'local, JThrowable<'local>> for Exception {
    type Output = Auto<'local, JThrowable<'local>>;

    fn lookup(self, env: &mut Env<'local>) -> Result<Self::Output> {
        let jni_class: JNIString = self.class.into();
        let jni_msg: JNIString = self.msg.into();
        Desc::<JThrowable>::lookup((jni_class, jni_msg), env)
    }
}

unsafe impl<'local> Desc<'local, JThrowable<'local>> for &str {
    type Output = Auto<'local, JThrowable<'local>>;

    fn lookup(self, env: &mut Env<'local>) -> Result<Self::Output> {
        let jni_msg: JNIString = self.into();
        Desc::<JThrowable>::lookup((DEFAULT_EXCEPTION_CLASS, jni_msg), env)
    }
}

unsafe impl<'local> Desc<'local, JThrowable<'local>> for String {
    type Output = Auto<'local, JThrowable<'local>>;

    fn lookup(self, env: &mut Env<'local>) -> Result<Self::Output> {
        let jni_msg: JNIString = self.into();
        Desc::<JThrowable>::lookup((DEFAULT_EXCEPTION_CLASS, jni_msg), env)
    }
}

unsafe impl<'local, T> Desc<'local, JThrowable<'local>> for T
where
    T: AsRef<JNIStr>,
{
    type Output = Auto<'local, JThrowable<'local>>;

    fn lookup(self, env: &mut Env<'local>) -> Result<Self::Output> {
        Desc::<JThrowable>::lookup((DEFAULT_EXCEPTION_CLASS, self.as_ref()), env)
    }
}
