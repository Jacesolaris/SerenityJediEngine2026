# SerenityJediEngine2026 launcher (Windows only): SINGLEPLAYER / MULTIPLAYER / OPTIONS / EXIT window that
# starts SerenityJediEngine2026-SP.x86_64.exe or SerenityJediEngine2026-MP.x86_64.exe from its own folder.
# art/launcher_bg.png is built by MD-work/sje-launcher/make_art.ps1 from the SP main menu background
# (SerenityJediEngine2026-Assets-0.pk3: gfx/menus/main_opening.JPG); the icon by make_icon.ps1 from the
# Jedi emblem icon (win32/SerenityJediEngine2026-glsl-SP.ico) in green.
# The sounds are the SJE main menu's own (SerenityJediEngine2026-Assets-1.pk3: sound/interface/menuroam.mp3,
# decoded to sound/menuroam.wav by mp3towav.ps1, and music/MainMenuTheme.mp3).

set(LauncherName "SerenityJediEngine2026-Launcher")

set(LauncherFiles
	"${SPDir}/launcher/sje_launcher.cpp"
	"${SPDir}/launcher/sje_launcher.rc"
	"${SPDir}/launcher/resource.h"
	)
source_group("launcher" FILES ${LauncherFiles})

add_executable(${LauncherName} WIN32 ${LauncherFiles})
target_compile_definitions(${LauncherName} PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(${LauncherName} PRIVATE "${SPDir}")
target_link_libraries(${LauncherName} gdiplus shlwapi winmm shell32)
set_target_properties(${LauncherName} PROPERTIES PROJECT_LABEL "Launcher")

install(TARGETS ${LauncherName}
	RUNTIME
	DESTINATION ${JKAInstallDir}
	COMPONENT ${JKASPClientComponent})
