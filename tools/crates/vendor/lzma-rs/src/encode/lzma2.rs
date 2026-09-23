use byteorder::{BigEndian, WriteBytesExt};
use std::io;

pub fn encode_stream<R, W>(input: &mut R, output: &mut W) -> io::Result<()>
where
    R: io::BufRead,
    W: io::Write,
{
    let mut buf = vec![0u8; 0x10000];
    loop {
        let n = input.read(&mut buf)?;
        if n == 0 {
            // status = EOF
            output.write_u8(0)?;
            break;
        }

        // status = uncompressed reset dict
        output.write_u8(1)?;
        // unpacked size
        output.write_u16::<BigEndian>((n - 1) as u16)?;
        // contents
        output.write_all(&buf[..n])?;
    }
    Ok(())
}
