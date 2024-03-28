#![allow(clippy::needless_range_loop)]

use libsecp256k1_core::curve::ECMultGenContext;
use std::{
    fs::File,
    io::{Error, Write},
};

pub fn generate_to(file: &mut File) -> Result<(), Error> {
    let context = ECMultGenContext::new_boxed();
    let prec = context.inspect_raw().as_ref();

    file.write_fmt(format_args!("["))?;
    for j in 0..64 {
        file.write_fmt(format_args!("    ["))?;
        for i in 0..16 {
            let pg = prec[j][i];
            file.write_fmt(format_args!(
                "        crate::curve::AffineStorage::new(crate::curve::FieldStorage::new({}, {}, {}, {}, {}, {}, {}, {}), crate::curve::FieldStorage::new({}, {}, {}, {}, {}, {}, {}, {})),",
                pg.x.0[7], pg.x.0[6], pg.x.0[5], pg.x.0[4], pg.x.0[3], pg.x.0[2], pg.x.0[1], pg.x.0[0],
                pg.y.0[7], pg.y.0[6], pg.y.0[5], pg.y.0[4], pg.y.0[3], pg.y.0[2], pg.y.0[1], pg.y.0[0]
            ))?;
        }
        file.write_fmt(format_args!("    ],"))?;
    }
    file.write_fmt(format_args!("]"))?;

    Ok(())
}
