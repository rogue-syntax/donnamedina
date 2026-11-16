# fw2svg: Fireworks PNG to SVG

Converts a Fireworks CS PNG (the editable kind, with the private `mkTS`/`mkBT` chunks) to SVG, so the
panel art (`fwrksrc/bkg8_donnaMedina3.png`) can be edited and exported without Fireworks.

    python fw2svg.py ../../fwrksrc/bkg8_donnaMedina3.png out.svg
    python cmp.py ../../fwrksrc/bkg8_donnaMedina3.png out.svg <prefix>   # render with Edge, diff with the flattened image

Output in `fwrksrc/svg/`: `bkg8_donnaMedina3.svg` (1148x542, the document) and
`bkg8_donnaMedina3_1248x542.svg` (the same with viewBox "35 15 1248 542": the panels where the editor
draws them since the -35,-15 move; the strip right of x 1113 is empty, the code draws the right column).

## The format (as far as needed here)
- `mkTS`: zlib-compressed document text. Items are `TAG` (3 letters) + type + `{...}`: `v` list,
  `s` uint16 length + UTF-16BE, `i` hex int, `f` float, `b` 0/1 (`fwparse.py`).
  `MKBv` > `LYLv` (layers, file order = bottom first) > `LAYv` (`LNMs` name, `VIFv/VISb` visible) >
  `CLLv/CELv` > `ELMv` (elements, file order = top first): `PTHv` path, `IMGv` bitmap, `TXTv` text,
  `GRPv` group (Auto Shapes are groups too).
- Path: `PBLv/PBPv` (`ISCb` closed) of `PBTv` points: anchor `XLC/YLC`, in-handle `XPC/YPC`, out-handle
  `XSC/YSC`. `PATv`: stroke colour `BCLi`, fill colour `FCLi` (ARGB), `FPLv` fill (`CATs` fc_Solid /
  fc_Linear; gradient stops `FGLv/FGVv/FGIv` `FGPf` position, `FGCi` colour; direction start `PSX/PSY`
  -> end `PEX/PEY`), `BPLv` brush (`BDIi` width), `TXFv/MSKv` fill texture (8-bit mask, row stride
  `TSZi`), texture amount `FPLv/FTBi` in tenths of a percent, offset `TOX/TOY`. `OPAi` opacity in
  tenths of a percent.
- Bitmaps: `IMGv` `XLC/YLC`, `WPX/HPX`, 128x128 tiles (`TILv/TIDi`) stored in the `mkBT` chunks
  (8-byte id header, zlib, ARGB, straight alpha).
- Text: `TXTv` with style runs `TFSv` (`TRNs` text, `FONs` font, `PTSf` size in px, `BOLb`, `ITLb`, `FCLi`
  colour, `KRNf` tracking in tenths of an em), alignment `JSTi` (0 left, 1 centre, 2 right, 3 justified),
  baseline anchor `LFA/TOA` (left edge, centre or right edge by `JST`), optional `MTXv` matrix
  (`M00 M01 M10 M11 M20 M21`), plus a cached rendering as an `IMGv`.
  Text is written as real SVG `<text>` (sharp at any scale; needs the fonts installed: Corbel, Courier New,
  Lucida Console; DejaVu Sans Mono falls back to Consolas). "Courier" was the Windows bitmap font, drawn at
  1.5x Courier New's size. Justified or multi-line text keeps the cached bitmap; `FW_TEXT=bitmap` uses the
  cached bitmaps for all text.
- `EFDv` is Fireworks' list of available effects, not effects applied to elements.

## Fidelity (against the flattened image in the same file)
Mean difference 1.81/255, 1.22 % of pixels off by more than 40 (0.97 and 0.40 % with `FW_TEXT=bitmap`; the
rest is the browser's text anti-aliasing against Fireworks'). Known differences: the heavier "Crosshatch 3"
texture is slightly less saturated, anti-aliasing of the knob tick rings and some gradient edges. The
knob tick wells are bitmaps in the source, so they do not get sharper at 2x.
