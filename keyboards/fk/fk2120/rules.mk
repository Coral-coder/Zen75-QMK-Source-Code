# Build Options
#   change yes to no to disable
#
# Board: it should exist either in <chibios>/os/hal/boards/
#  or <this_dir>/boards
BOARD = FS026
NO_USB_STARTUP_CHECK = yes
BLUETOOTH_CUSTOM = yes

DEBOUNCE_TYPE= sym_defer_g

# The vendor library (lib/rdr_lib) calls into VIA / dynamic keymap code and
# provides the EEPROM driver, so every keymap needs these.
VIA_ENABLE = yes
EEPROM_DRIVER = custom

# Extra lighting effects, see rgb_matrix_kb.inc
RGB_MATRIX_CUSTOM_KB = yes
