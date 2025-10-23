use crate::result::{ZipError, ZipResult};
use crate::unstable::{LittleEndianReadExt, LittleEndianWriteExt};
use std::borrow::Cow;
use std::io;
use std::io::prelude::*;
use std::path::{Component, Path, MAIN_SEPARATOR};

pub const LOCAL_FILE_HEADER_SIGNATURE: u32 = 0x04034b50;
pub const CENTRAL_DIRECTORY_HEADER_SIGNATURE: u32 = 0x02014b50;
pub(crate) const CENTRAL_DIRECTORY_END_SIGNATURE: u32 = 0x06054b50;
pub const ZIP64_CENTRAL_DIRECTORY_END_SIGNATURE: u32 = 0x06064b50;
pub(crate) const ZIP64_CENTRAL_DIRECTORY_END_LOCATOR_SIGNATURE: u32 = 0x07064b50;

pub const ZIP64_BYTES_THR: u64 = u32::MAX as u64;
pub const ZIP64_ENTRY_THR: usize = u16::MAX as usize;

pub struct CentralDirectoryEnd {
    pub disk_number: u16,
    pub disk_with_central_directory: u16,
    pub number_of_files_on_this_disk: u16,
    pub number_of_files: u16,
    pub central_directory_size: u32,
    pub central_directory_offset: u32,
    pub zip_file_comment: Vec<u8>,
}

impl CentralDirectoryEnd {
    pub fn parse<T: Read>(reader: &mut T) -> ZipResult<CentralDirectoryEnd> {
        let magic = reader.read_u32_le()?;
        if magic != CENTRAL_DIRECTORY_END_SIGNATURE {
            return Err(ZipError::InvalidArchive("Invalid digital signature header"));
        }
        let disk_number = reader.read_u16_le()?;
        let disk_with_central_directory = reader.read_u16_le()?;
        let number_of_files_on_this_disk = reader.read_u16_le()?;
        let number_of_files = reader.read_u16_le()?;
        let central_directory_size = reader.read_u32_le()?;
        let central_directory_offset = reader.read_u32_le()?;
        let zip_file_comment_length = reader.read_u16_le()? as usize;
        let mut zip_file_comment = vec![0; zip_file_comment_length];
        reader.read_exact(&mut zip_file_comment)?;

        Ok(CentralDirectoryEnd {
            disk_number,
            disk_with_central_directory,
            number_of_files_on_this_disk,
            number_of_files,
            central_directory_size,
            central_directory_offset,
            zip_file_comment,
        })
    }

    pub fn find_and_parse<T: Read + Seek>(reader: &mut T) -> ZipResult<(CentralDirectoryEnd, u64)> {
        const HEADER_SIZE: u64 = 22;
        const BYTES_BETWEEN_MAGIC_AND_COMMENT_SIZE: u64 = HEADER_SIZE - 6;
        let file_length = reader.seek(io::SeekFrom::End(0))?;

        let search_upper_bound = file_length.saturating_sub(HEADER_SIZE + u16::MAX as u64);

        if file_length < HEADER_SIZE {
            return Err(ZipError::InvalidArchive("Invalid zip header"));
        }

        let mut pos = file_length - HEADER_SIZE;
        while pos >= search_upper_bound {
            reader.seek(io::SeekFrom::Start(pos))?;
            if reader.read_u32_le()? == CENTRAL_DIRECTORY_END_SIGNATURE {
                reader.seek(io::SeekFrom::Current(
                    BYTES_BETWEEN_MAGIC_AND_COMMENT_SIZE as i64,
                ))?;
                let cde_start_pos = reader.seek(io::SeekFrom::Start(pos))?;
                if let Ok(end_header) = CentralDirectoryEnd::parse(reader) {
                    return Ok((end_header, cde_start_pos));
                }
            }
            pos = match pos.checked_sub(1) {
                Some(p) => p,
                None => break,
            };
        }
        Err(ZipError::InvalidArchive(
            "Could not find central directory end",
        ))
    }

    pub fn write<T: Write>(&self, writer: &mut T) -> ZipResult<()> {
        writer.write_u32_le(CENTRAL_DIRECTORY_END_SIGNATURE)?;
        writer.write_u16_le(self.disk_number)?;
        writer.write_u16_le(self.disk_with_central_directory)?;
        writer.write_u16_le(self.number_of_files_on_this_disk)?;
        writer.write_u16_le(self.number_of_files)?;
        writer.write_u32_le(self.central_directory_size)?;
        writer.write_u32_le(self.central_directory_offset)?;
        writer.write_u16_le(self.zip_file_comment.len() as u16)?;
        writer.write_all(&self.zip_file_comment)?;
        Ok(())
    }
}

pub struct Zip64CentralDirectoryEndLocator {
    pub disk_with_central_directory: u32,
    pub end_of_central_directory_offset: u64,
    pub number_of_disks: u32,
}

impl Zip64CentralDirectoryEndLocator {
    pub fn parse<T: Read>(reader: &mut T) -> ZipResult<Zip64CentralDirectoryEndLocator> {
        let magic = reader.read_u32_le()?;
        if magic != ZIP64_CENTRAL_DIRECTORY_END_LOCATOR_SIGNATURE {
            return Err(ZipError::InvalidArchive(
                "Invalid zip64 locator digital signature header",
            ));
        }
        let disk_with_central_directory = reader.read_u32_le()?;
        let end_of_central_directory_offset = reader.read_u64_le()?;
        let number_of_disks = reader.read_u32_le()?;

        Ok(Zip64CentralDirectoryEndLocator {
            disk_with_central_directory,
            end_of_central_directory_offset,
            number_of_disks,
        })
    }

    pub fn write<T: Write>(&self, writer: &mut T) -> ZipResult<()> {
        writer.write_u32_le(ZIP64_CENTRAL_DIRECTORY_END_LOCATOR_SIGNATURE)?;
        writer.write_u32_le(self.disk_with_central_directory)?;
        writer.write_u64_le(self.end_of_central_directory_offset)?;
        writer.write_u32_le(self.number_of_disks)?;
        Ok(())
    }
}

pub struct Zip64CentralDirectoryEnd {
    pub version_made_by: u16,
    pub version_needed_to_extract: u16,
    pub disk_number: u32,
    pub disk_with_central_directory: u32,
    pub number_of_files_on_this_disk: u64,
    pub number_of_files: u64,
    pub central_directory_size: u64,
    pub central_directory_offset: u64,
    //pub extensible_data_sector: Vec<u8>, <-- We don't do anything with this at the moment.
}

impl Zip64CentralDirectoryEnd {
    pub fn find_and_parse<T: Read + Seek>(
        reader: &mut T,
        nominal_offset: u64,
        search_upper_bound: u64,
    ) -> ZipResult<Vec<(Zip64CentralDirectoryEnd, u64)>> {
        let mut results = Vec::new();
        let mut pos = search_upper_bound;

        while pos >= nominal_offset {
            reader.seek(io::SeekFrom::Start(pos))?;

            if reader.read_u32_le()? == ZIP64_CENTRAL_DIRECTORY_END_SIGNATURE {
                let archive_offset = pos - nominal_offset;

                let _record_size = reader.read_u64_le()?;
                // We would use this value if we did anything with the "zip64 extensible data sector".

                let version_made_by = reader.read_u16_le()?;
                let version_needed_to_extract = reader.read_u16_le()?;
                let disk_number = reader.read_u32_le()?;
                let disk_with_central_directory = reader.read_u32_le()?;
                let number_of_files_on_this_disk = reader.read_u64_le()?;
                let number_of_files = reader.read_u64_le()?;
                let central_directory_size = reader.read_u64_le()?;
                let central_directory_offset = reader.read_u64_le()?;

                results.push((
                    Zip64CentralDirectoryEnd {
                        version_made_by,
                        version_needed_to_extract,
                        disk_number,
                        disk_with_central_directory,
                        number_of_files_on_this_disk,
                        number_of_files,
                        central_directory_size,
                        central_directory_offset,
                    },
                    archive_offset,
                ));
            }
            if pos > 0 {
                pos -= 1;
            } else {
                break;
            }
        }
        if results.is_empty() {
            Err(ZipError::InvalidArchive(
                "Could not find ZIP64 central directory end",
            ))
        } else {
            Ok(results)
        }
    }

    pub fn write<T: Write>(&self, writer: &mut T) -> ZipResult<()> {
        writer.write_u32_le(ZIP64_CENTRAL_DIRECTORY_END_SIGNATURE)?;
        writer.write_u64_le(44)?; // record size
        writer.write_u16_le(self.version_made_by)?;
        writer.write_u16_le(self.version_needed_to_extract)?;
        writer.write_u32_le(self.disk_number)?;
        writer.write_u32_le(self.disk_with_central_directory)?;
        writer.write_u64_le(self.number_of_files_on_this_disk)?;
        writer.write_u64_le(self.number_of_files)?;
        writer.write_u64_le(self.central_directory_size)?;
        writer.write_u64_le(self.central_directory_offset)?;
        Ok(())
    }
}

/// Converts a path to the ZIP format (forward-slash-delimited and normalized).
pub(crate) fn path_to_string<T: AsRef<Path>>(path: T) -> String {
    let mut maybe_original = None;
    if let Some(original) = path.as_ref().to_str() {
        if (MAIN_SEPARATOR == '/' || !original[1..].contains(MAIN_SEPARATOR))
            && !original.ends_with('.')
            && !original.starts_with(['.', MAIN_SEPARATOR])
            && !original.starts_with(['.', '.', MAIN_SEPARATOR])
            && !original.contains([MAIN_SEPARATOR, MAIN_SEPARATOR])
            && !original.contains([MAIN_SEPARATOR, '.', MAIN_SEPARATOR])
            && !original.contains([MAIN_SEPARATOR, '.', '.', MAIN_SEPARATOR])
        {
            if original.starts_with(MAIN_SEPARATOR) {
                maybe_original = Some(&original[1..]);
            } else {
                maybe_original = Some(original);
            }
        }
    }
    let mut recreate = maybe_original.is_none();
    let mut normalized_components = Vec::new();

    // Empty element ensures the path has a leading slash, with no extra allocation after the join
    normalized_components.push(Cow::Borrowed(""));

    for component in path.as_ref().components() {
        match component {
            Component::Normal(os_str) => match os_str.to_str() {
                Some(valid_str) => normalized_components.push(Cow::Borrowed(valid_str)),
                None => {
                    recreate = true;
                    normalized_components.push(os_str.to_string_lossy());
                }
            },
            Component::ParentDir => {
                recreate = true;
                if normalized_components.len() > 1 {
                    normalized_components.pop();
                }
            }
            _ => {
                recreate = true;
            }
        }
    }
    if recreate {
        normalized_components.join("/")
    } else {
        drop(normalized_components);
        let original = maybe_original.unwrap();
        if !original.starts_with('/') {
            let mut slash_original = String::with_capacity(original.len() + 1);
            slash_original.push('/');
            slash_original.push_str(original);
            slash_original
        } else {
            original.to_string()
        }
    }
}
