#[doc(hidden)]
pub struct len<const N: usize>;

#[doc(hidden)]
pub struct char<const char: core::primitive::char>;

macro_rules! letters {
    ($($letter:ident)*) => {
        $(
            #[doc(hidden)]
            pub type $letter = char<{
                stringify!($letter).as_bytes()[0] as core::primitive::char
            }>;
        )*
    };
}

letters! {
    A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
    a b c d e f g h i j k l m n o p q r s t u v w x y z
}

#[doc(hidden)]
pub mod two {
    #[doc(hidden)]
    pub struct char<const char: core::primitive::char>;
}

#[doc(hidden)]
pub mod three {
    #[doc(hidden)]
    pub struct char<const char: core::primitive::char>;
}

#[doc(hidden)]
pub mod four {
    #[doc(hidden)]
    pub struct char<const char: core::primitive::char>;
}
