# How registration works at Rogue Syntax

## Set up once

1. **One installer recipe.** Rogue Syntax owns one Inno Setup script for every
   plugin; each plugin fills in only its name, file and registry folder. It's
   in the repo's `installer/` folder.
2. **Private key** makes licences. It's on the server:
   `/etc/rogue-syntax/license_signing_key`. Only the owner has it.
3. **Public key** checks licences. It's in every plugin's code:
   `7a51af6239b6ed75c695f100cde294f9745a06bad932ed870236dbf4e6578b78`

## Each purchase

4. **Customer buys.** The server uses the private key to make that customer's
   **licence key**: their details (plugin, name, email, date, readable) plus a
   **signature** that proves the store made it and nobody changed it.
5. **Server delivers it.** The licence key goes into `license.txt`, zipped with
   the installer, and into the receipt email.
6. **Customer runs the installer.** It installs the plugin and copies the
   key from `license.txt`. If it couldn't, the plugin shows "Not registered"
   and the customer clicks Paste license.
7. **Backend is done.** It only hands out the same key again: re-download or
   resent receipt email.

## Packaging a plugin for the store

1. **Operator sends the pack.** Operator dashboard → "Plugin developer pack" →
   download the zip and send it. Each plugin's name tag is already in it.
2. **Developer builds the installer** with it (steps in its README) and sends
   back `<Plugin>Setup.exe`.
3. **Operator puts it in the store:** dashboard → Products → "upload new"
   on that product's row (add the product first if it's new).

Installers are **not code-signed** for now, so Windows shows "Unknown
publisher" (More info → Run anyway). Buy a certificate later to remove it.

## Every plugin must

1. **Read the key** from the registry:
   `HKEY_CURRENT_USER\Software\Rogue-Syntax\Registrations\<Plugin>`, value
   `License`.
2. **Check it** with the public key. It must be genuine and for this plugin.
3. **Registered:** work normally; the About window says "Licensed to <name>".
4. **Not registered:** play no notes and show a **"Not registered"** window
   with two buttons:
   - **Buy**: opens the plugin's page on rogue-syntax.com.
   - **Paste license**: the customer pastes their key; the plugin checks it,
     saves it to the registry and unlocks.
5. **Check for updates** once a day when it opens, in the background:
   - send the store its product ID, version, licence key (if registered;
     the store checks it, so it can't be faked) and an anonymous computer ID (a scrambled code from the PC, never its
     name);
   - the store replies with the latest version; if it's newer, show
     "Update available" with a link to download it;
   - offline or no answer within a few seconds: carry on silently. The check
     never stops the plugin working;
   - customers can turn it off (a setting in the About window).
   - The exact contract (address, fields, reply, rules) is in
     [licensing-v1.md](licensing-v1.md) section 1.8.

## Who uses what

The store keeps each update check: licence number, computer ID, version, last
seen. The operator dashboard shows how many computers each licence is used on
(e.g. "AB-000123: 14 computers"), to spot shared keys.
