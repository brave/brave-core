use crate::decode;
use crate::encode::{lzma2, util};
use crate::xz::crc::CRC32;
use crate::xz::{footer, header, CheckMethod, StreamFlags};
use byteorder::{LittleEndian, WriteBytesExt};
use std::io;
use std::io::Write;

pub fn encode_stream<R, W>(input: &mut R, output: &mut W) -> io::Result<()>
where
    R: io::BufRead,
    W: io::Write,
{
    let stream_flags = StreamFlags {
        check_method: CheckMethod::None,
    };

    // Header
    write_header(output, stream_flags)?;

    // Block
    let (unpadded_size, unpacked_size) = write_block(input, output)?;

    // Index
    let index_size = write_index(output, unpadded_size, unpacked_size)?;

    // Footer
    write_footer(output, stream_flags, index_size)
}

fn write_header<W>(output: &mut W, stream_flags: StreamFlags) -> io::Result<()>
where
    W: io::Write,
{
    output.write_all(header::XZ_MAGIC)?;
    let mut digest = CRC32.digest();
    {
        let mut digested = util::CrcDigestWrite::new(output, &mut digest);
        stream_flags.serialize(&mut digested)?;
    }
    let crc32 = digest.finalize();
    output.write_u32::<LittleEndian>(crc32)?;
    Ok(())
}

fn write_footer<W>(output: &mut W, stream_flags: StreamFlags, index_size: usize) -> io::Result<()>
where
    W: io::Write,
{
    let mut digest = CRC32.digest();
    let mut footer_buf: Vec<u8> = Vec::new();
    {
        let mut digested = util::CrcDigestWrite::new(&mut footer_buf, &mut digest);

        let backward_size = (index_size >> 2) - 1;
        digested.write_u32::<LittleEndian>(backward_size as u32)?;
        stream_flags.serialize(&mut digested)?;
    }
    let crc32 = digest.finalize();
    output.write_u32::<LittleEndian>(crc32)?;
    output.write_all(footer_buf.as_slice())?;

    output.write_all(footer::XZ_MAGIC_FOOTER)?;
    Ok(())
}

fn write_block<R, W>(input: &mut R, output: &mut W) -> io::Result<(usize, usize)>
where
    R: io::BufRead,
    W: io::Write,
{
    let (unpadded_size, unpacked_size) = {
        let mut count_output = util::CountWrite::new(output);

        // Block header
        let mut digest = CRC32.digest();
        {
            let mut digested = util::CrcDigestWrite::new(&mut count_output, &mut digest);
            let header_size = 8;
            digested.write_u8((header_size >> 2) as u8)?;
            let flags = 0x00; // 1 filter, no (un)packed size provided
            digested.write_u8(flags)?;
            let filter_id = 0x21; // LZMA2
            digested.write_u8(filter_id)?;
            let size_of_properties = 1;
            digested.write_u8(size_of_properties)?;
            let properties = 22; // TODO
            digested.write_u8(properties)?;
            let padding = [0, 0, 0];
            digested.write_all(&padding)?;
        }
        let crc32 = digest.finalize();
        count_output.write_u32::<LittleEndian>(crc32)?;

        // Block
        let mut count_input = decode::util::CountBufRead::new(input);
        lzma2::encode_stream(&mut count_input, &mut count_output)?;
        (count_output.count(), count_input.count())
    };
    lzma_info!(
        "Unpadded size = {}, unpacked_size = {}",
        unpadded_size,
        unpacked_size
    );

    let padding_size = ((unpadded_size ^ 0x03) + 1) & 0x03;
    let padding = vec![0; padding_size];
    output.write_all(padding.as_slice())?;
    // Checksum = None (cf. above)

    Ok((unpadded_size, unpacked_size))
}

fn write_index<W>(output: &mut W, unpadded_size: usize, unpacked_size: usize) -> io::Result<usize>
where
    W: io::Write,
{
    let mut count_output = util::CountWrite::new(output);

    let mut digest = CRC32.digest();
    {
        let mut digested = util::CrcDigestWrite::new(&mut count_output, &mut digest);
        digested.write_u8(0)?; // No more block
        let num_records = 1;
        write_multibyte(&mut digested, num_records)?;

        write_multibyte(&mut digested, unpadded_size as u64)?;
        write_multibyte(&mut digested, unpacked_size as u64)?;
    }

    // Padding
    let count = count_output.count();
    let padding_size = ((count ^ 0x03) + 1) & 0x03;
    {
        let mut digested = util::CrcDigestWrite::new(&mut count_output, &mut digest);
        let padding = vec![0; padding_size];
        digested.write_all(padding.as_slice())?;
    }

    let crc32 = digest.finalize();
    count_output.write_u32::<LittleEndian>(crc32)?;

    Ok(count_output.count())
}

fn write_multibyte<W>(output: &mut W, mut value: u64) -> io::Result<()>
where
    W: io::Write,
{
    loop {
        let byte = (value & 0x7F) as u8;
        value >>= 7;
        if value == 0 {
            output.write_u8(byte)?;
            break;
        } else {
            output.write_u8(0x80 | byte)?;
        }
    }

    Ok(())
}
