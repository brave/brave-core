#[derive(num_enum::TryFromPrimitive)]
#[repr(u8)]
enum Numbers {
    Zero = 0,
    #[num_enum(alternatives = [..255])]
    NonZero = 1,
}

fn main() {

}
