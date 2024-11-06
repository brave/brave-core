use cfg_if::cfg_if;

cfg_if! {
    if #[cfg(feature = "debug_trace")] {
        macro_rules! trace {
            ( @actions $($actions:tt)+ ) => ( println!("@action: {}", stringify!($($actions)+)); );

            ( @chars $action_descr:expr $(, $ch:expr)* ) => {
                print!(">{}", $action_descr);

                $({
                    use std::char;

                    print!(": {:?}", $ch.map(|ch| unsafe { char::from_u32_unchecked(ch as u32) }));
                })*

                println!();
            };

            ( @buffer $buffer:expr ) => {
                use crate::base::Bytes;

                println!("-- Buffered: {:#?}", Bytes::from($buffer.bytes()));
            };

            ( @write $slice:expr ) => {
                use crate::base::Bytes;

                println!("-- Write: {:#?}", Bytes::from($slice));
            };

            ( @end ) => ( println!("-- End"); );

            ( @chunk $chunk:expr ) => {
                println!();
                println!("{:#?}", $chunk);
                println!();
            };

            ( @noop ) => ( println!("NOOP"); );

            ( @continue_from_bookmark $bookmark:expr, $parser_directive:expr, $chunk:expr ) => {
                use crate::base::Bytes;

                println!();
                println!("Continue from:");
                println!("{:#?}", $bookmark);
                println!("Parser directive: `{:#?}`", $parser_directive);

                let mut chunk_str = Bytes::from($chunk).as_debug_string();

                chunk_str.insert_str($bookmark.pos, "|*|");

                println!("Bookmark start: `{}`", chunk_str);
                println!();
            };

            ( @output $output:expr ) => {
                println!();
                println!("{:#?}", $output);
                println!();
            };
        }
    } else {
        macro_rules! trace {
            ( @$ty:ident $($args:tt)* ) => {};
            ( @$ty:ident $($args:tt),* ) => {};
        }
    }
}
