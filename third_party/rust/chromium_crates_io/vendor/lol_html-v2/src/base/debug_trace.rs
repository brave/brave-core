use cfg_if::cfg_if;

cfg_if! {
    if #[cfg(feature = "debug_trace")] {
        macro_rules! trace {
            ( @actions $($actions:tt)+ ) => ( println!("@action: {}", stringify!($($actions)+)); );

            ( @chars $action_descr:expr $(, $ch:expr)* ) => {
                print!(">{}", $action_descr);

                $({
                    use std::char;

                    print!(": {:?}", $ch.map(|ch| char::from_u32(ch as u32).unwrap_or('\u{fffd}') ));
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

                // as_debug_string() is UTF-8, and the position for the input encoding is not guaranteed to match it
                let chunk = Bytes::from($chunk);
                let (before, after) = chunk.split_at($bookmark.pos);

                println!("Bookmark start: `{}|*|{}`", before.as_debug_string(), after.as_debug_string());
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
