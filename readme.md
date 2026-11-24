# Zen75 QMK firmware

QMK/VIA firmware source for the Cerakey Zen75 (`fk/fk2120`, FS026 / ES32 MCU, USB + Bluetooth + 2.4 GHz).
It is the vendor's QMK tree plus fixes, extra lighting effects and a CI build.
The upstream QMK readme is kept in [docs/qmk_readme.md](docs/qmk_readme.md).

> The wireless, battery, EEPROM and LED-driver code ships only as a prebuilt library
> (`lib/rdr_lib/librdrcommon.a`). It cannot be rebuilt or changed from this repository.

## Lighting

### Changing colours

| Keys | Action |
| --- | --- |
| `Fn` + `PgUp` / `PgDn` | Hue + / − |
| `Fn` + `Home` / `End` | Saturation + / − (all the way down gives white) |
| `Fn` + `F6` / `F5` | Brightness + / − |
| `Fn` + `←` / `→` | Next / previous effect |
| `Fn` + `↑` / `↓` | Speed + / − |
| `Fn` + `Enter` | Lighting on / off |

Side light bar: `Fn` + `Z` on/off, `X` next effect, `C`/`V` hue +/−, `B`/`N` saturation +/−,
`M`/`,` brightness +/−, `.`/`/` speed +/−.

In VIA (load `keyboards/fk/fk2120/Zen75@Cerakey.JSON` in the Design tab) the Lighting page has a
colour picker for the keys and a separate **Side Light** section.

Hue and saturation apply to every effect except the fully random ones (`jellybean_raindrops`,
`pixel_flow`) and `digital_rain`, which is deliberately left as the stock green "matrix" effect.
The rainbow effects ignore hue but follow saturation.

### Added effects

They come after the stock effects in the cycle (numbers are the VIA effect ids).

| # | Effect | |
| --- | --- | --- |
| 46 | `nasa_worm` | The NASA worm lettering scrolling slowly right to left |
| 47 | `digital_rain_hue` | The matrix rain in the selected colour |
| 48 | `fire` | Flames from the bottom row; the hue sets the ember colour |
| 49 | `aurora` | Slow curtains of colour around the selected hue |
| 50 | `scanner` | A bar sweeping side to side |

Source: [`keyboards/fk/fk2120/rgb_matrix_kb.inc`](keyboards/fk/fk2120/rgb_matrix_kb.inc).

## Building

```sh
sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi binutils-arm-none-eabi make
python3 -m pip install qmk -r requirements.txt
make fk/fk2120:via SKIP_GIT=yes
```

The image is written to `.build/fk_fk2120_via.bin`. It must stay under 126976 bytes (0x1F000),
where the emulated EEPROM begins; CI checks this.

`fk_fk2120_via_stock.bin` is the vendor's original build, kept so you can always go back.

## Releases

Pushing any tag that ends in `b` (for example `v0.3.0b`) runs
[`.github/workflows/build.yml`](.github/workflows/build.yml), which builds the firmware and
publishes a GitHub release with the `.bin`, the VIA definition and checksums. Pull requests get
the same build as a downloadable artifact.

If the new `Fn` shortcuts do not respond after flashing, the keyboard is still using the keymap
stored in its EEPROM: reset it (`Fn` + `Backspace`, held for about three seconds) or assign the
keys in VIA.

None of the changes in this repository have been tested on hardware by CI; builds are compile
and size checked only.
