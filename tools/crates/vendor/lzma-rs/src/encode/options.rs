/// Options for the `lzma_compress` function
#[derive(Clone, Copy, Debug, Default)]
pub struct Options {
    /// Defines whether the unpacked size should be written to the header.
    /// The default is
    /// [`UnpackedSize::WriteToHeader(None)`](enum.encode.UnpackedSize.html#variant.WriteValueToHeader)
    pub unpacked_size: UnpackedSize,
}

/// Alternatives for handling unpacked size
#[derive(Clone, Copy, Debug)]
pub enum UnpackedSize {
    /// If the value is `Some(u64)`, write the provided u64 value to the header.
    /// There is currently no check in place that verifies that this is the actual number of bytes
    /// provided by the input stream.
    /// If the value is `None`, write the special `0xFFFF_FFFF_FFFF_FFFF` code to the header,
    /// indicating that the unpacked size is unknown.
    WriteToHeader(Option<u64>),
    /// Do not write anything to the header. The unpacked size needs to be stored elsewhere and
    /// provided when reading the file. Note that this is a non-standard way of writing LZMA data,
    /// but is used by certain libraries such as
    /// [OpenCTM](http://openctm.sourceforge.net/).
    SkipWritingToHeader,
}

impl Default for UnpackedSize {
    fn default() -> UnpackedSize {
        UnpackedSize::WriteToHeader(None)
    }
}
