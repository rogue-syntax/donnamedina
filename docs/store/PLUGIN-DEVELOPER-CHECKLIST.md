# Plugin developer checklist

Everything you need is in this zip:
- `registration-journey.md`: how registration works, in plain steps;
- `licensing-v1.md`: the exact rules (sections 1 and 2 are yours);
- `README.md` and the `.iss` files: the installer recipe.

## Your plugin's identity

Work these out from your plugin's name (spec 1.7), then confirm them with
the store owner. They never change once the plugin ships.

- Product ID
- Registry subkey (`PluginKey`)
- Licence prefix
- Shop page (the Buy button)
- Installer ID (`AppGuid`)

## Steps

1. **Licence check:** the plugin checks licences with the public key
   `7a51af6239b6ed75c695f100cde294f9745a06bad932ed870236dbf4e6578b78`
   (spec 2.1).
2. **Tests:** test vectors T1–T7 and the reject cases pass (spec 1.6, 2.5).
3. **Not registered:** no notes, and a window with **Buy** and **Paste
   license** (spec 2.2, 2.3).
4. **Registered:** About says "Licensed to <name>" (spec 2.3).
5. **Update check:** once a day, in the background; badge, About line, off
   switch (spec 1.8).
6. **Dev licence:** ask the store owner for a `dev` licence for your plugin,
   paste it, check the plugin unlocks (spec 2.6).
7. **Installer:** build `<PluginKey>Setup.exe` with this recipe
   (`README.md`). Test it: put a `license.txt` next to it, run it, open the
   plugin. It should be licensed without pasting.
8. **Hand over:** send the store owner the `.exe` and its version number
   (e.g. `1.0.0`).
