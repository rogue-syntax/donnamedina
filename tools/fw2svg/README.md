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
  (8-byte id header, zlib, ARGB, straight alpha). Text elements carry a cached rendering as an `IMGv`,
  which is used as is.
- `EFDv` is Fireworks' list of available effects, not effects applied to elements.

## Fidelity (against the flattened image in the same file)
Mean difference 0.97/255, 0.40 % of pixels off by more than 40. Known differences: the heavier
"Crosshatch 3" texture is slightly less saturated, anti-aliasing of the knob tick rings and some
gradient edges. Text and the knob tick wells are bitmaps in the source, so they do not get sharper at 2x.
