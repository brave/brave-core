/* exhaustively search for these symbols */
EXTERN(_defmt_acquire);
EXTERN(_defmt_release);
EXTERN(__defmt_default_timestamp);
EXTERN(__DEFMT_MARKER_TIMESTAMP_WAS_DEFINED);
PROVIDE(_defmt_timestamp = __defmt_default_timestamp);
PROVIDE(_defmt_panic = __defmt_default_panic);

SECTIONS
{

  /* `1` specifies the start address of this virtual (`(INFO)`) section */
  /* Tag number 0 is reserved for special uses, like as a format sequence terminator. */
  .defmt 1 (INFO) :
  {
    /* For some reason the `1` above has no effect, but this does */
    . = 1;

    /* Format implementations for primitives like u8 */
    *(.defmt.prim.*);

    /* We order the ids of the log messages by severity and put markers in between, so that we can filter logs at runtime by severity */
    __DEFMT_MARKER_TRACE_START = .;
    *(.defmt.trace.*);
    __DEFMT_MARKER_TRACE_END = .;
    __DEFMT_MARKER_DEBUG_START = .;
    *(.defmt.debug.*);
    __DEFMT_MARKER_DEBUG_END = .;
    __DEFMT_MARKER_INFO_START = .;
    *(.defmt.info.*);
    __DEFMT_MARKER_INFO_END = .;
    __DEFMT_MARKER_WARN_START = .;
    *(.defmt.warn.*);
    __DEFMT_MARKER_WARN_END = .;
    __DEFMT_MARKER_ERROR_START = .;
    *(.defmt.error.*);
    __DEFMT_MARKER_ERROR_END = .;

    /* Everything user-defined */
    *(.defmt.*);

    __DEFMT_MARKER_END = .;

    /* Symbols that aren't referenced by the program and */
    /* should be placed at the end of the section */
    KEEP(*(.defmt.end .defmt.end.*));
  }
}

ASSERT(__DEFMT_MARKER_END < 65534, ".defmt section cannot contain more than 65534 interned strings");
