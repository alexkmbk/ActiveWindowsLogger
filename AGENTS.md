# Agent Notes

## GitHub Release Workflow

Use this checklist when preparing a GitHub release.

1. Set the release version in `WinSetup/ProductVersion.txt`.
   - Use MSI-compatible three-part versions such as `1.4.0`.
   - `WinSetup/make.bat` reads this file and passes it to WiX as `ProductVersion`.

2. Build the x86 application executable.
   - The MSI packages `Release/ActiveWindowsLogger.exe`.
   - Build the Visual Studio solution as `Release|x86` before packaging.

3. Rebuild the MSI.
   - Run from `WinSetup`:

```bat
make.bat
```

   - The output is `WinSetup/ActiveWindowsLogger.msi`.
   - The MSI version should match `WinSetup/ProductVersion.txt`.
   - The MSI is per-user, installs to `%LOCALAPPDATA%\ActiveWindowsLogger`, writes autostart to `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`, and supports major upgrades through the stable `UpgradeCode`.

4. Prepare GitHub release assets.
   - `WinSetup/ActiveWindowsLogger.msi`
   - `Release/ActiveWindowsLogger.exe`, uploaded as `ActiveWindowsLogger_portable.exe`
   - `Viewer/1C/ActiveWindowsLogsViewer.epf`

5. Create or update the GitHub release.
   - Tags use plain versions, for example `1.4.0`.
   - Release title format: `ActiveWindowsLogger 1.4.0`.
   - If replacing assets in an existing release, use `gh release upload <version> <file> --clobber`.

6. Update the Infostart package zip.
   - Rebuild `InfostartPackage/ActiveWindowsLogger.zip` with exactly these files:
     - `ActiveWindowsLogger.msi`
     - `ActiveWindowsLogger_portable.exe`
     - `ActiveWindowsLogsViewer.epf`
   - `InfostartPackage/ActiveWindowsLogger.zip` is ignored by Git, so verify its contents locally after rebuilding.

7. Do not accidentally stage unrelated local changes.
   - In particular, check `git status --short` before committing.
   - Build outputs under `Release`, `WinSetup/*.msi`, `WinSetup/*.wixpdb`, and `InfostartPackage/*.zip` are ignored.
