// Prints the license test vectors in docs/07-licensing.md (TEST key only, seed 00..1f). Run: go run tools/license/testvectors.go
package main

import (
	"crypto/ed25519"
	"encoding/base64"
	"encoding/hex"
	"fmt"
	"strings"
)

var b64 = base64.RawURLEncoding

func token(priv ed25519.PrivateKey, payload string) string {
	sig := ed25519.Sign(priv, []byte(payload))
	return "DM1." + b64.EncodeToString([]byte(payload)) + "." + b64.EncodeToString(sig)
}

func p(kid, lid, kind, name, email, issued, product string) string {
	return "v=1\nproduct=" + product + "\nkid=" + kid + "\nlid=" + lid + "\nkind=" + kind +
		"\nname=" + name + "\nemail=" + email + "\nissued=" + issued + "\n"
}

func main() {
	seed := make([]byte, 32)
	for i := range seed {
		seed[i] = byte(i)
	}
	priv := ed25519.NewKeyFromSeed(seed)
	pub := priv.Public().(ed25519.PublicKey)
	fmt.Println("seed_hex", hex.EncodeToString(seed))
	fmt.Println("pub_hex", hex.EncodeToString(pub))
	full := token(priv, p("0", "DM-TEST-0001", "full", "Jane Q. Tester", "jane@example.com", "2026-11-19", "donnamedina"))
	fmt.Println("T1_valid_full", full)
	fmt.Println("T2_valid_dev", token(priv, p("0", "DM-TEST-0002", "dev", "Rogue Syntax Dev", "dev@example.com", "2026-11-19", "donnamedina")))
	fmt.Println("T3_valid_utf8", token(priv, p("0", "DM-TEST-0003", "full", "Zoë Müller = DJ", "zoe@example.com", "2026-11-19", "donnamedina")))
	// tampered: re-encode a payload with a different name but keep T1's signature
	parts := strings.Split(full, ".")
	fake := b64.EncodeToString([]byte(p("0", "DM-TEST-0001", "full", "Someone Else", "jane@example.com", "2026-11-19", "donnamedina")))
	fmt.Println("T4_tampered", "DM1."+fake+"."+parts[2])
	fmt.Println("T5_wrong_product", token(priv, p("0", "DM-TEST-0005", "full", "Jane Q. Tester", "jane@example.com", "2026-11-19", "othersynth")))
	fmt.Println("T6_unknown_kid", token(priv, p("9", "DM-TEST-0006", "full", "Jane Q. Tester", "jane@example.com", "2026-11-19", "donnamedina")))
	// wrapped like an email client would: line breaks every 40 chars plus surrounding spaces
	var w strings.Builder
	w.WriteString("  ")
	for i := 0; i < len(full); i += 40 {
		e := i + 40
		if e > len(full) {
			e = len(full)
		}
		w.WriteString(full[i:e])
		w.WriteString("\r\n")
	}
	fmt.Printf("T7_wrapped_hex %s\n", hex.EncodeToString([]byte(w.String())))
	// verify round trip
	pl, _ := b64.DecodeString(parts[1])
	sg, _ := b64.DecodeString(parts[2])
	fmt.Println("selfcheck", ed25519.Verify(pub, pl, sg))
}
