# Plugin developer checklist

Everything you need is in this zip:
- `registration-journey.md`: how registration works, in plain steps;
- `licensing-v1.md`: the exact rules (sections 1 and 2 are yours);
- `README.md`, `build.ps1` and `rogue-syntax.iss`: the installer recipe;
- `product.json`: your plugin's identity and the version to beat.

## Your plugin's identity

It's in `product.json` (spec 1.7, 1.9). Use those values exactly; they never
change once the plugin ships:

- Product ID (`product_id`)
- Registry subkey (`registry_key`)
- Licence prefix (`licence_prefix`)
- Shop page, the Buy button (`shop_page`)
- Installer ID (`app_guid`)

## Steps

1. **Licence check:** the plugin checks licences with the public key
   `7a51af6239b6ed75c695f100cde294f9745a06bad932ed870236dbf4e6578b78`
   (spec 2.1).
2. **Tests:** test vectors T1–T7 and the reject cases pass (spec 1.6, 2.5).
3. **Not registered:** no notes, and a window with **Buy** and **Paste
   license** (spec 2.2, 2.3).
4. **Registered:** About says "Licensed to <name>" (spec 2.3).
5. **Names:** wherever the plugin shows its own name or the company's, write
   them as `<Plugin name>™` and `Rogue Syntax Sound Design™`.
6. **Update check:** once a day, in the background; badge, About line, off
   switch (spec 1.8).
7. **Dev licence:** ask the store owner for a `dev` licence for your plugin,
   paste it, check the plugin unlocks (spec 2.6).
8. **Release package:** build it with `build.ps1` (`README.md`). Test the
   installer in `output\`: put a `license.txt` next to it, run it, open the
   plugin. It should be licensed without pasting.
9. **Hand over:** send the store owner `output\<registry_key>-<version>.zip`.
   The store checks it (spec 1.9) and says what to fix if it's refused.
