<p align="center">
  <img src="assets/EchoPatch_Logo.png" style="max-width:75%">
</p>

Modernizes F.E.A.R. and its expansions with HUD scaling, high-framerate optimizations, controller support, and other quality-of-life enhancements. It aims to be as non-intrusive as possible, with no file modifications and no gameplay changes by default, focusing solely on fixing issues and enhancing the overall experience.

## How to Install
> [!NOTE]  
> Compatible with F.E.A.R. Ultimate Shooter Edition (Steam) and F.E.A.R. Platinum (GOG).  
> All features are compatible with Extraction Point and Perseus Mandate.  
>
> **Download**: [EchoPatch.zip](https://github.com/Wemino/EchoPatch/releases/latest/download/EchoPatch.zip)  
> Extract the contents of the zip file into the game’s folder, in the same directory as the `FEAR.exe` file.  
> On first launch, EchoPatch will prompt to apply a [LAA patch](#laa-patcher) if needed to prevent loading issues.

> [!WARNING]
> The GOG version defaults to a 60 FPS cap.  
> To unlock higher framerates, modify the `dxwrapper.ini` file by setting `LimitPerFrameFPS` from **60** to **0**.  
> This change enables compatibility with the `HighFPSFixes` optimizations, ensuring smooth performance at framerates up to 240 FPS.

### Steam Deck/Linux Specific Instructions (Windows users can skip this)
> [!WARNING]
> To launch the game on Steam Deck or Linux, open the game’s properties in Steam and include `WINEDLLOVERRIDES="dinput8=n,b" %command%` in the launch options.
> 
> On Steam Deck, change the controller configuration to `Gamepad With Joystick Trackpad` for controller support.

# Features

## HUD Scaling
- Dynamically scales HUD elements (texts, crosshair, icons) relative to screen resolution.  
- Adjust `HUDCustomScalingFactor` in `EchoPatch.ini` to customize overall HUD scaling.
- Adjust `SmallTextCustomScalingFactor` for independent scaling of smaller text (e.g., subtitles).

<div align="center">
  <table>
    <tr>
      <td width="50%"><img style="width:100%" src="https://raw.githubusercontent.com/Wemino/EchoPatch/refs/heads/main/assets/4K_HUD_Normal.png"></td>
      <td width="50%"><img style="width:100%" src="https://raw.githubusercontent.com/Wemino/EchoPatch/refs/heads/main/assets/4K_HUD_Scaled.png"></td>
    </tr>
    <tr>
      <td align="center">4K Vanilla</td>
      <td align="center">4K EchoPatch</td>
    </tr>
  </table>
</div>

> **Note**: The base resolution (1024×768) is used as the reference for scaling, ensuring the HUD retains its original proportions and appearance on all higher resolutions.

## Fix High FPS Issues
Resolves multiple issues at high framerates, designed and optimized for smooth gameplay at up to 240 FPS (particularly when using Slow-Mo):
- Ragdoll physics instability.
- Water physics instability.
- Excessive water splash effect repetitions.
- Frozen FX effects.
- Oversized particles.
- Overly dampened velocity when jumping out of water.
- Velocity dampening when jumping and landing.
- Walking animation prematurely reverting to idle, causing camera stutter.
- Inability to perform a jump kick.
- Excessive sliding on sloped surfaces.

## Optimized Save Performance
Dramatically reduces save times by buffering file operations in memory instead of writing directly to disk.  
The game performs hundreds of thousands of individual WriteFile calls per save (over 220,000 for a typical 2MB save file), causing multi-second delays even on high-end hardware.

## Input & Frame Drop Fixes
- **FPS Drop Fix**: Stops the game from initializing all HID devices as a controller to prevent framerate drops over time, rather than intercepting the call as in other fixes.  
- **Input Lag Fix**: Disables the `SetWindowsHookEx` call to reduce input lag.

## Fix Rendering Corruption on Nvidia GPUs
Resolves rendering issues such as shadow flickering and inversion on Nvidia GPUs.  
This issue appeared in Nvidia drivers released after 2015 and persists in modern drivers, with a small performance trade-off for correct shadow rendering.

## Framerate Limiter
Prevents the game from running too fast by capping the maximum framerate.  
- **MaxFPS** (`MaxFPS` in `EchoPatch.ini`): Set the maximum framerate. A value of `0` disables the limiter, any other value enables it. The default value of `240` is the recommended safe value, as some high FPS optimizations may not cover higher framerates.  
- **Dynamic VSync** (`DynamicVsync` in `EchoPatch.ini`): When enabled (`1`), VSync synchronizes frame updates to your monitor’s refresh rate, reducing screen tearing. VSync will only be enabled if your monitor’s refresh rate is lower than `MaxFPS`, otherwise it remains off. Set to `0` to disable.  

> **Note**: Can be disabled by setting `FixNvidiaShadowCorruption = 0` in `EchoPatch.ini` if wanted.

## Weapon Fixes
Addresses several weapon-related issues:
- Fix Aim/Zoom not working when loading a save during a cutscene.
- Automatic weapons stuck in firing animation when loading a save made mid-fire.
- Weapon cycling not working when there’s an empty slot between two weapons.
- Weapon model position not updating correctly after reloading a save with the same weapon equipped.

> **Note**: These issues were partially fixed in the Extraction Point and Perseus Mandate expansions.

## Controller Support

Supports Xbox, PlayStation (DualShock 3/4, DualSense), and Nintendo Switch controllers via SDL3.

| Controller Input                 | Action                         |
|----------------------------------|--------------------------------|
| **Left Analog Stick**            | Move                           |
| **Right Analog Stick**           | Aim                            |
| **Left Analog Stick** (Press)    | Use Health Kit                 |
| **Right Analog Stick** (Press)   | Toggle Flashlight On/Off       |
| **A / Cross**                    | Jump                           |
| **B / Circle**                   | Crouch                         |
| **X / Square**                   | Reload / Interact / Pick Up    |
| **Y / Triangle**                 | Toggle Slow-Motion             |
| **RT / R2 / ZR**                 | Fire                           |
| **LT / L2 / ZL**                 | Zoom                           |
| **RB / R1 / R**                  | Melee                          |
| **LB / L1 / L**                  | Throw Grenade                  |
| **D-Pad Up**                     | Next Weapon                    |
| **D-Pad Down**                   | Next Grenade                   |
| **D-Pad Left**                   | Lean Left                      |
| **D-Pad Right**                  | Lean Right                     |
| **Back / Share / −**             | Mission Status                 |

### Additional Feature (PlayStation controllers)
- **Touchpad**: Mouse cursor control (DualShock 4/DualSense)

Customizable alongside sensitivity settings within the `[Controller]` section of `EchoPatch.ini`.
> **Note**: Hotplugging is supported, connect or disconnect controllers at any time without restarting the game.  
> **Note**: For a more console-like experience, you can automatically hide the mouse cursor when a controller is detected. This feature is disabled by default. To enable it, set `HideMouseCursor=1` in `EchoPatch.ini`.  
> **Note**: Custom controller mappings can be added via `gamecontrollerdb.txt`.

## Fix Keyboard Input Initialization
Corrects default control assignment on non‑English layouts by mapping hardware scan codes instead of English key names (preventing some “[unassigned]” entries on first launch or after resetting controls) while leaving the saved bindings in the save file unchanged.

## Widescreen Resolution Support for Extraction Point
Removes 4:3 restriction so all widescreen resolutions are available.

## LAA Patcher
Applies a Large Address Aware patch to allow up to 4 GB of memory (default 2 GB), which can resolve loading issues such as the "Disconnected from server" error.  
The behavior of this option can be edited by setting `CheckLAAPatch` in the `[Fixes]` section of `EchoPatch.ini`.

## Persistent World State
Keeps objects (bodies, blood stains, debris, bullet holes, shell casings, glass shards…) from despawning.

## HD Reflections & Displays
Improves resolution quality of reflective surfaces and displays.

<div align="center">
  <table>
    <tr>
      <td width="50%"><img style="width:100%" src="https://raw.githubusercontent.com/Wemino/EchoPatch/refs/heads/main/assets/HD_Render_Off.png"></td>
      <td width="50%"><img style="width:100%" src="https://raw.githubusercontent.com/Wemino/EchoPatch/refs/heads/main/assets/HD_Render_On.png"></td>
    </tr>
    <tr>
      <td align="center">Vanilla</td>
      <td align="center">HD Display</td>
    </tr>
  </table>
</div>

## No Model LOD Bias
Renders the highest quality models at all distances by disabling LOD bias.

## Reduced Mipmap Bias
Improves texture sharpness at a distance by reducing mipmap bias.

## Mouse Aim Multiplier
Multiplier applied to mouse aiming to compensate for high sensitivity (does not affect profile settings).  
Set `MouseAimMultiplier` in `EchoPatch.ini` (default `1.0`).

## Disable Letterboxing
Disables cutscene letterboxing when `DisableLetterbox = 1` in `EchoPatch.ini`.

<div align="center">
  <table>
    <tr>
      <td width="50%"><img style="width:100%" src="https://raw.githubusercontent.com/Wemino/EchoPatch/refs/heads/main/assets/Letterbox_On.png"></td>
      <td width="50%"><img style="width:100%" src="https://raw.githubusercontent.com/Wemino/EchoPatch/refs/heads/main/assets/Letterbox_Off.png"></td>
    </tr>
    <tr>
      <td align="center">Vanilla</td>
      <td align="center">Letterbox Disabled</td>
    </tr>
  </table>
</div>

## Auto Resolution
Automatically sets the game window to match your screen resolution on first launch (or every launch if `AutoResolution = 2`).

## Skip Splashscreen
Bypasses developer splash on launch when `SkipSplashScreen = 1`.

## Skip Movies
Skips intro videos when `SkipAllIntro = 1`.  
Individual videos can be skipped via the `SkipIntro` section in `EchoPatch.ini`.

## Disable PunkBuster Initialization
Disables initialization of PunkBuster, the deprecated online anti-cheat service that shipped with the original game.  
This prevents the game from loading unused background components.

## Save Folder Redirection
Redirects the save folder from `%PUBLIC%\Documents\` to `%USERPROFILE%\Documents\My Games\`.  
Disabled by default, set `RedirectSaveFolder = 1` in `EchoPatch.ini` to enable.

## Infinite Flashlight
Removes battery limit and hides HUD indicator.  
Enable with `InfiniteFlashlight = 1`.

## Weapon Capacity Editor
Customize max weapon capacity via `MaxWeaponCapacity` (0–10).  
Enable with `EnableCustomMaxWeaponCapacity = 1`.  

<img src="assets/WeaponCapacity.png" style="max-width:75%">

> **Note:** Weapon capacity is tied to the save data. Lowering the capacity will not take effect on an existing save. Start a new save file to apply a reduced limit.

## dinput8 Chaining Support
Chains another `dinput8.dll` by loading `dinput8_hook.dll` for mod compatibility.

## Configuration
All features can be customized via the `EchoPatch.ini` file. Each setting includes detailed comments explaining its function and acceptable values. The patch uses sensible defaults that work for most users, but allows fine-tuning of every aspect.

---

## Credits
- [SDL3](https://www.libsdl.org/) for controller support.
- [MinHook](https://github.com/TsudaKageyu/minhook) for hooking.  
- [mINI](https://github.com/metayeti/mINI) for INI file handling.  
- [Methanhydrat](https://community.pcgamingwiki.com/files/file/789-directinput-fps-fix/) for identifying the FPS drop root cause.  
- [Vityacv](https://github.com/Vityacv) for identifying the extra latency caused by SetWindowsHookEx.
- [CRASHARKI](https://github.com/CRASHARKI) for the logo.



