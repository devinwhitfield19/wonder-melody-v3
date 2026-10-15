# Build the Windows VST3 in GitHub Actions (no local compiler needed)

This workflow attempts to compile the plugin on a hosted Windows runner, so Visual Studio, CMake, and JUCE do not need to be installed on your own PC. A GitHub account is required, and the workflow has not yet been run; the first build may expose source errors that need fixing.

1. Create/sign in to a GitHub account at https://github.com/.
2. Create a new repository. A private repository is fine.
3. Upload the contents of this project folder to the repository (the `.github/workflows/windows-vst3.yml` file must be included at the repository root under `.github/workflows/`).
4. Open the repository's **Actions** tab.
5. Select **Build Windows VST3** and click **Run workflow**.
6. If the run succeeds, open that run and download the artifact named `Wonder-Melody-Generator-Windows-VST3`.
7. Extract the artifact. Copy the included `.vst3` bundle into `%LOCALAPPDATA%\Programs\Common\VST3` (create the folder if needed), or another VST3 folder FL Studio scans.
8. In FL Studio, open **Options → Manage plugins** and run a scan. Search for **Wonder Melody Generator**.

If the workflow fails, open the failed run and inspect the first compiler/configuration error. This project is not yet verified in FL Studio, and a successful compile alone does not guarantee all MIDI/audio behavior is correct.
