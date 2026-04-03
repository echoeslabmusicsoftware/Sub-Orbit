# Installing SUB ORBIT

## Download

Grab the latest release for your platform from the [Releases page](https://github.com/echoeslabmusicsoftware/Sub-Orbit/releases).

---

## macOS

1. Download `SubOrbit-macOS-Installer.pkg`
2. Double-click the `.pkg` file
3. Follow the installer prompts — both VST3 and AU plugins are installed to the system plugin directories
4. Rescan plugins in your DAW

The installer places plugins in:
- **VST3** → `/Library/Audio/Plug-Ins/VST3/`
- **AU** → `/Library/Audio/Plug-Ins/Components/`

### Security warning

These builds are not notarized by Apple, so macOS will block the installer on first launch. To open it:

1. Right-click (or Control-click) the `.pkg` file
2. Select **Open** from the context menu
3. Click **Open** in the dialog

Alternatively, remove the quarantine flag from the terminal:
```bash
xattr -cr ~/Downloads/SubOrbit-macOS-Installer.pkg
```

This is a one-time step.

---

## Windows

1. Download `SubOrbit-Windows-Installer.exe`
2. Run the installer
3. Click through the setup wizard — the VST3 plugin is installed to `C:\Program Files\Common Files\VST3\`
4. Rescan plugins in your DAW

### SmartScreen warning

These builds are not signed with a Windows code signing certificate. When SmartScreen shows a warning:

1. Click **More info**
2. Click **Run anyway**

---

## Linux

### Option 1: .deb package (Debian/Ubuntu)

```bash
sudo dpkg -i suborbit_*_amd64.deb
```

The VST3 plugin is installed to `/usr/lib/vst3/`.

### Option 2: Manual install

1. Extract `SubOrbit-Linux-VST3.tar.gz`
2. Move `SUB ORBIT.vst3` to `~/.vst3/` or `/usr/lib/vst3/`
3. Rescan plugins in your DAW

---

## Uninstalling

**macOS:** Delete the plugin files from `/Library/Audio/Plug-Ins/VST3/SUB ORBIT.vst3` and `/Library/Audio/Plug-Ins/Components/SUB ORBIT.component`.

**Windows:** Use **Add or Remove Programs** in Windows Settings.

**Linux (deb):** `sudo dpkg -r suborbit`

**Linux (manual):** Delete the `SUB ORBIT.vst3` folder from wherever you placed it.

---

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
