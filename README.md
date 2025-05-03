<p align="center">
 <img style="width:75%" src="assets/EchoPatch_Logo.png"">
</p>

Modernizes F.E.A.R. and its expansions with HUD scaling, high-framerate optimizations, XInput controller support, and other quality-of-life enhancements.

## How to Install
> [!NOTE]
> Compatible with F.E.A.R. Ultimate Shooter Edition (Steam) and F.E.A.R. Platinum (GOG).
> 
> All features are compatible with Extraction Point and Perseus Mandate.
> 
> **Download**: [EchoPatch.zip](https://github.com/Wemino/EchoPatch/releases/latest/download/EchoPatch.zip)
>
> Extract the contents of the zip file into the game’s folder, in the same directory as the `FEAR.exe` file.

> [!NOTE]
> The GOG version defaults to a 60 FPS cap.
>
> To unlock higher framerates, modify the `dxwrapper.ini` file by setting `LimitPerFrameFPS` from **60** to **0**.
>
> This change enables compatibility with the `HighFPSFixes` optimizations, ensuring smooth performance at framerates up to 240 FPS.

## Features
 - **HUD Scaling** - This feature dynamically scales HUD elements (such as texts, the crosshair, and icons) relative to the screen resolution. The base resolution is 960x720, ensuring the HUD retains its original proportions and appearance on all higher resolutions.

The `HUDCustomScalingFactor` setting in `EchoPatch.ini` can be adjusted to customize the overall scaling of HUD elements.

The `SmallTextCustomScalingFactor` setting allows for independent scaling of smaller text elements, such as subtitles.

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

- **Fix High FPS issues** - Addresses instability in physics, effects, and gameplay mechanics at high framerates, resolving issues such as erratic ragdoll behavior, unstable visual effects, and movement inaccuracies.

- **Framerate Limiter** - The game engine struggles with very low delta times at high framerates. Since it lacks a framerate cap, this can cause the game to run too fast. This feature lets you set a maximum framerate to prevent that. To adjust the limit, edit `MaxFPS` in `EchoPatch.ini`. A maximum value of 240 when `HighFPSFixes` is enabled is recommended.

- **FPS Drop Fix** - Stop the game from initializing all HID devices, which leads to framerate drops over time. The difference with Methanhydrat's `dinput8.dll` fix is that, rather than intercepting and canceling the problematic call, the call will simply never be executed. This method is specifically effective for F.E.A.R. and does not apply to the other games affected by this issue.

- **Fix Keyboard Input Initialization** - Fixes key mapping issues in non-English systems by correctly initializing keyboard input settings, preventing keys from showing as "[unassigned]" in the controls mapping.

- **LAA Patcher** - Apply a Large Address Aware (LAA) patch when necessary, enabling the use of up to 4GB of memory instead of the default 2GB, which can resolve loading issues. Disabled by default, set `CheckLAAPatch` to 1 to enable. Note: If you are using the Steam version, please run [Steamless](https://github.com/atom0s/Steamless) on `FEAR.exe` before enabling this feature.

- **Persistent World State** - Enables persistent world state by keeping certain objects in the world instead of allowing them to despawn. Examples include: bodies, blood stains, debris, bullet holes, shell casings, glass shards...

- **HD Reflections** - Increases the resolution quality of reflective surfaces (e.g., water reflections) and improves the clarity of security monitor video feeds.

- **No Model LOD Bias** - Forces the highest quality models to be rendered at all distances.

- **No Mipmap Bias** - Forces the highest quality textures to be rendered at all distances. Disabled by default, set `NoMipMapBias` to 1 to enable. (may cause visual artifacts such as shimmering)
<div align="center">
  <table>
    <tr>
      <td width="100%"><img style="width:100%" src="https://raw.githubusercontent.com/Wemino/EchoPatch/refs/heads/main/assets/MipMapBias.png"></td>
    </tr>
    <tr>
      <td align="center">MipMapBias settings: On vs. Off at 1920x1080</td>
    </tr>
  </table>
</div>

- **Disable Letterboxing** - Disable the letterbox during cutscenes when `DisableLetterbox` is set to 1 in `EchoPatch.ini`.
<div align="center">
  <table>
    <tr>
      <td width="50%"><img style="width:100%" src="https://raw.githubusercontent.com/Wemino/EchoPatch/refs/heads/main/assets/Letterbox_On.png"></td>
      <td width="50%"><img style="width:100%" src="https://raw.githubusercontent.com/Wemino/EchoPatch/refs/heads/main/assets/Letterbox_Off.png"></td>
    </tr>
    <tr>
      <td align="center">Vanilla</td>
      <td align="center">EchoPatch (Letterbox Disabled)</td>
    </tr>
  </table>
</div>

- **Widescreen Resolution Support for Extraction Point** – In the Extraction Point expansion, only 4:3 aspect ratio resolutions are shown in the resolution list. This restriction has been removed.

- **Auto Resolution** - Automatically sets the game window size to match the screen resolution on the first launch.

- **Skip Splashscreen** - Bypasses developer splash screen immediately on launch when `SkipSplashScreen` is set to 1 in `EchoPatch.ini`.

- **Skip Movies** - Skips all corporate intro videos while keeping the sound for the menu when `SkipAllIntro` is set to 1 in `EchoPatch.ini`. Additionally, individual videos can be skipped instead of all; refer to the `SkipIntro` section in `EchoPatch.ini`.

- **Infinite Flashlight** - Removes the flashlight battery limit and hides the HUD indicator. Disabled by default, can be enabled by setting `InfiniteFlashlight = 1` in `EchoPatch.ini`.

- **dinput8 Chaining Support** - Allows chaining another `dinput8.dll` file by loading `dinput8_hook.dll`, enabling compatibility with additional mods.

- **XInput Controller Support** - Enables full compatibility with XInput-based controllers: 

<div align="center">
 
| Controller Input                 | Action                         |
|----------------------------------|--------------------------------|
| **Left Analog Stick**            | Move                           |
| **Right Analog Stick**           | Aim                            |
| **Left Analog Stick** (Press)    | Use Health Kit                 |
| **Right Analog Stick** (Press)   | Toggle Flashlight On/Off       |
| **A Button**                     | Jump                           |
| **B Button**                     | Crouch                         |
| **X Button**                     | Reload / Interact / Pick Up    |
| **Y Button**                     | Toggle Slow-Motion             |
| **Right Trigger (RT)**           | Fire                           |
| **Left Trigger (LT)**            | Zoom                           |
| **Right Bumper (RB)**            | Melee                          |
| **Left Bumper (LB)**             | Throw Grenade                  |
| **D-Pad Up**                     | Next Weapon                    |
| **D-Pad Down**                   | Next Grenade                   |
| **D-Pad Left**                   | Lean Left                      |
| **D-Pad Right**                  | Lean Right                     |
| **Back Button**                  | Mission Status                 |

(Customizable under the ini section of `[Controller]`)
</div>

## Credits
- [MinHook](https://github.com/TsudaKageyu/minhook) for hooking.
- [mINI](https://github.com/metayeti/mINI) for INI file handling.
- [Methanhydrat](https://community.pcgamingwiki.com/files/file/789-directinput-fps-fix/) for finding the root cause of the FPS drop issue.
- [Vityacv](https://github.com/Vityacv) for identifying the extra latency caused by SetWindowsHookEx.
- [CRASHARKI](https://github.com/CRASHARKI) for the logo.
