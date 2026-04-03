# Installing SUB ORBIT

## Download

Grab the latest release for your platform from the [Releases page](https://github.com/echoeslabmusic/sub-orbit-plugin/releases).

## Verify your download (optional)

Each release includes `SHA256SUMS.txt`. Compare checksums to confirm the file hasn't been tampered with:

```bash
# macOS / Linux
sha256sum -c SHA256SUMS.txt

# macOS (if sha256sum not available)
shasum -a 256 -c SHA256SUMS.txt
```

```powershell
# Windows (PowerShell)
Get-Content SHA256SUMS.txt | ForEach-Object {
  $parts = $_ -split '  '
  $expected = $parts[0]; $file = $parts[1]
  $actual = (Get-FileHash $file -Algorithm SHA256).Hash.ToLower()
  if ($actual -eq $expected) { "OK: $file" } else { "MISMATCH: $file" }
}
```

---

## macOS

These builds are not notarized by Apple, so macOS will show a security warning. This is normal for independent software distributed outside the App Store.

### VST3

1. Unzip `SubOrbit-macOS-VST3.zip`
2. Move `SUB ORBIT.vst3` to `/Library/Audio/Plug-Ins/VST3/` (all users) or `~/Library/Audio/Plug-Ins/VST3/` (current user only)
3. Remove the quarantine flag:
   ```bash
   xattr -cr /Library/Audio/Plug-Ins/VST3/SUB\ ORBIT.vst3
   ```
4. Rescan plugins in your DAW

### AU (Audio Unit)

1. Unzip `SubOrbit-macOS-AU.zip`
2. Move `SUB ORBIT.component` to `/Library/Audio/Plug-Ins/Components/` (all users) or `~/Library/Audio/Plug-Ins/Components/` (current user only)
3. Remove the quarantine flag:
   ```bash
   xattr -cr /Library/Audio/Plug-Ins/Components/SUB\ ORBIT.component
   ```
4. Rescan plugins in your DAW (you may need to restart it)

### Why is this necessary?

macOS quarantines all downloaded software and blocks anything not signed with an Apple Developer ID certificate. The `xattr -cr` command removes this quarantine attribute. It's a one-time step — you won't need to do it again unless you re-download.

---

## Windows

These builds are not signed with a Windows code signing certificate, so SmartScreen will show a warning on first run.

### VST3

1. Unzip `SubOrbit-Windows-VST3.zip`
2. Move the `SUB ORBIT.vst3` folder to `C:\Program Files\Common Files\VST3\`
3. Rescan plugins in your DAW

---

## Linux

No special steps needed — Linux does not enforce code signing for audio plugins.

### VST3

1. Extract `SubOrbit-Linux-VST3.tar.gz`
2. Move `SUB ORBIT.vst3` to `~/.vst3/` or `/usr/lib/vst3/`
3. Rescan plugins in your DAW

