/*
===========================================================================
SerenityJediEngine2027 launcher
===========================================================================

A small window like the Jedi Academy launcher: SINGLEPLAYER, MULTIPLAYER, OPTIONS and EXIT.
MULTIPLAYER opens its own list: START MULTIPLAYER, START SERVER and BACK. START SERVER runs
"Start SerenityJediEngine-MP-FFA-Server.bat" from the SerenityJediEngine2027 mod folder in its own console window
and the launcher stays open (so the game can be started next, to join the server).
It starts SerenityJediEngine2027-SP.x86_64.exe or SerenityJediEngine2027-MP.x86_64.exe from its own
folder, passing the options chosen on the OPTIONS page as "+set" command line values (they override
the configs), and then closes. The options are remembered in SerenityJediEngine2027-Launcher.ini in
Documents\STARWARS SerenityJediEngine\SerenityJediEngine2027, next to the game's own configs and saves.

The background (the SP main menu art), the menu sound and the main menu music are built into the exe,
see sje_launcher.rc. The buttons are drawn here in the SJE blue. The music loops while the launcher is
open; the speaker button next to minimise switches it off (remembered in the ini).
*/

#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <gdiplus.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <mmsystem.h>

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "resource.h"
#include "win32/AutoVersion.h"

namespace
{
	constexpr int kWidth = 1088;
	constexpr int kHeight = 607;

	constexpr int kButtonCount = 4;
	constexpr int kButtonW = 320;
	constexpr int kButtonH = 36;
	constexpr int kButtonGap = 8;
	constexpr int kButtonTop = 398;

	// hit ids besides the 4 menu buttons
	constexpr int kHitNone = -1;
	constexpr int kHitMinimise = 100;
	constexpr int kHitClose = 101;
	constexpr int kHitMusic = 102;

	// background music: MCI plays the MP3 (from a temp file), PlaySound the menu sound alongside
	const wchar_t* const kMusicAlias = L"sje_launcher_music";
	constexpr int kMusicVolume = 600; // 0..1000

	const wchar_t* const kSPExe = L"SerenityJediEngine2027-SP.x86_64.exe";
	const wchar_t* const kMPExe = L"SerenityJediEngine2027-MP.x86_64.exe";
	const wchar_t* const kIniName = L"SerenityJediEngine2027-Launcher.ini";
	// the mod folder (fs_game) next to the exes, and the dedicated server batch file in it (it does "cd .." itself, so it
	// runs from the mod folder)
	const wchar_t* const kModFolder = L"SerenityJediEngine2027";
	const wchar_t* const kServerBat = L"Start SerenityJediEngine-MP-FFA-Server.bat";
	// Release builds of the game only start when this is set on their process (shared/sys/sys_main.cpp
	// Sys_RequireLauncher: SJE_LAUNCHER_ENV / SJE_LAUNCHER_TOKEN); keep the two the same.
	const wchar_t* const kLauncherEnv = L"SJE_LAUNCHER";
	const wchar_t* const kLauncherToken = L"sje-launcher-4b8e2d6a";
	// the game's home folder in Documents, with its configs and saves (Sys_DefaultHomePath: "STARWARS SerenityJediEngine\"
	// + HOMEPATH_NAME_WIN)
	const wchar_t* const kHomeParent = L"STARWARS SerenityJediEngine";
	const wchar_t* const kHomeFolder = L"SerenityJediEngine2027";

	// SJE colours: the blue of the menu art and the icons
	const Gdiplus::Color kTextColor(255, 177, 194, 220);  // the main menu's button text (.695 .760 .861)
	const Gdiplus::Color kTextLit(255, 235, 245, 255);
	const Gdiplus::Color kEdgeColor(150, 70, 130, 200);
	const Gdiplus::Color kEdgeLit(235, 120, 200, 255);
	const Gdiplus::Color kGlow(45, 80, 170, 255);

	enum class Page { Main, Options, Multiplayer };

	// the game's config files (in the mod folder of the Documents home, else of the install folder)
	const wchar_t* const kSPConfig = L"SerenityJediEngine2027-SP.cfg";
	const wchar_t* const kMPConfig = L"SerenityJediEngine2027-MP.cfg";

	// resolutions the OPTIONS page cycles through (0 x 0 = the desktop resolution, r_mode -2)
	struct Resolution
	{
		int width;
		int height;
	};
	const Resolution kResolutions[] = { { 0, 0 },{ 1280, 720 },{ 1366, 768 },{ 1600, 900 },{ 1920, 1080 },{ 2560, 1440 },
		{ 3840, 2160 } };
	// the engine's fixed r_mode list (shared/sdl/sdl_window.cpp r_vidModes, r_mode 0..20)
	const Resolution kRModes[] = { { 320, 240 },{ 400, 300 },{ 512, 384 },{ 640, 480 },{ 800, 600 },{ 856, 480 },
		{ 960, 720 },{ 1024, 768 },{ 1152, 864 },{ 1280, 720 },{ 1280, 1024 },{ 1366, 768 },{ 1440, 900 },
		{ 1600, 900 },{ 1600, 1200 },{ 1680, 1050 },{ 1920, 1080 },{ 2048, 1536 },{ 2400, 600 },{ 2560, 1440 },
		{ 3840, 2160 } };

	struct Settings
	{
		bool rend2 = false;
		bool windowed = false;
		bool controller = true; // in_joystick: gamepad / joystick input (the game default is on)
		bool music = true;  // launcher background music (speaker button)
		Resolution resolution = { 0, 0 }; // { 0, 0 } = desktop
	};

	struct Launcher
	{
		HINSTANCE instance = nullptr;
		std::wstring folder; // where the launcher (and the game exes) are
		Settings settings;
		Page page = Page::Main;
		int hover = kHitNone;
		int pressed = kHitNone;
		bool tracking_mouse = false;

		std::wstring music_file; // the music written to %TEMP% for MCI
		bool music_open = false;

		std::unique_ptr<Gdiplus::Bitmap> background;
	};

	Launcher g_launcher;

	// ------------------------------------------------------------------------------------------
	// Resources
	// ------------------------------------------------------------------------------------------

	// The bytes of an RCDATA resource of the exe (valid while the program runs).
	bool GetResourceBytes(const int id, const void** bytes, DWORD* size)
	{
		const HRSRC res = FindResourceW(g_launcher.instance, MAKEINTRESOURCEW(id), MAKEINTRESOURCEW(10)); // RT_RCDATA
		const HGLOBAL data = res ? LoadResource(g_launcher.instance, res) : nullptr;
		*bytes = data ? LockResource(data) : nullptr;
		*size = res ? SizeofResource(g_launcher.instance, res) : 0;
		return *bytes && *size;
	}

	// MCI can only play music from a file: writes the resource to %TEMP%\<name>. Empty string on failure.
	std::wstring WriteResourceToTemp(const int id, const wchar_t* name)
	{
		const void* bytes;
		DWORD size;
		wchar_t temp[MAX_PATH] = {};
		if (!GetResourceBytes(id, &bytes, &size) || !GetTempPathW(MAX_PATH, temp))
		{
			return {};
		}
		const std::wstring file = std::wstring(temp) + name;
		const HANDLE h = CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
		if (h == INVALID_HANDLE_VALUE)
		{
			return {};
		}
		DWORD written = 0;
		const bool ok = WriteFile(h, bytes, size, &written, nullptr) && written == size;
		CloseHandle(h);
		if (!ok)
		{
			DeleteFileW(file.c_str());
			return {};
		}
		return file;
	}

	bool OpenMp3(const std::wstring& file, const wchar_t* alias)
	{
		const std::wstring open = L"open \"" + file + L"\" type mpegvideo alias " + alias;
		return mciSendStringW(open.c_str(), nullptr, 0, nullptr) == 0;
	}

	void CloseMp3(const wchar_t* alias)
	{
		const std::wstring close = std::wstring(L"close ") + alias;
		mciSendStringW(close.c_str(), nullptr, 0, nullptr);
	}

	void DeleteTempFile(std::wstring& file)
	{
		if (!file.empty())
		{
			DeleteFileW(file.c_str());
			file.clear();
		}
	}

	// ------------------------------------------------------------------------------------------
	// Sound
	// ------------------------------------------------------------------------------------------

	// The menu sound (hover, click and back, like the game's menus): a new one cuts off the one still
	// playing. wait: play it to the end first (for a click right before the launcher closes).
	void PlaySfx(const bool wait = false)
	{
		const void* bytes;
		DWORD size;
		if (GetResourceBytes(IDR_SND_MENU, &bytes, &size))
		{
			PlaySoundW(static_cast<LPCWSTR>(bytes), nullptr, SND_MEMORY | SND_NODEFAULT | (wait ? SND_SYNC : SND_ASYNC));
		}
	}

	// The music loops: when a play finishes, MCI sends MM_MCINOTIFY to the window, which starts it
	// again (see WndProc).
	void PlayMusicFromStart(const HWND hwnd)
	{
		const std::wstring cmd = std::wstring(L"play ") + kMusicAlias + L" from 0 notify";
		mciSendStringW(cmd.c_str(), nullptr, 0, hwnd);
	}

	void StartMusic(const HWND hwnd)
	{
		if (g_launcher.music_open || !g_launcher.settings.music)
		{
			return;
		}
		if (g_launcher.music_file.empty())
		{
			g_launcher.music_file = WriteResourceToTemp(IDR_MUSIC, L"SerenityJediEngine2027-Launcher-music.mp3");
		}
		if (g_launcher.music_file.empty() || !OpenMp3(g_launcher.music_file, kMusicAlias))
		{
			return; // no MP3 playback on this PC: just no music
		}
		g_launcher.music_open = true;
		const std::wstring volume = std::wstring(L"setaudio ") + kMusicAlias + L" volume to " + std::to_wstring(kMusicVolume);
		mciSendStringW(volume.c_str(), nullptr, 0, nullptr);
		PlayMusicFromStart(hwnd);
	}

	void StopMusic()
	{
		if (g_launcher.music_open)
		{
			CloseMp3(kMusicAlias);
			g_launcher.music_open = false;
		}
	}

	// ------------------------------------------------------------------------------------------
	// Art
	// ------------------------------------------------------------------------------------------

	// Loads a PNG from the exe's RCDATA resources.
	std::unique_ptr<Gdiplus::Bitmap> LoadPng(const int id)
	{
		const void* bytes;
		DWORD size;
		if (!GetResourceBytes(id, &bytes, &size))
		{
			return nullptr;
		}
		IStream* stream = SHCreateMemStream(static_cast<const BYTE*>(bytes), size);
		if (!stream)
		{
			return nullptr;
		}
		// the bitmap reads from the stream while it lives: copy it so the stream can go
		std::unique_ptr<Gdiplus::Bitmap> from_stream(Gdiplus::Bitmap::FromStream(stream));
		std::unique_ptr<Gdiplus::Bitmap> copy;
		if (from_stream && from_stream->GetLastStatus() == Gdiplus::Ok)
		{
			copy.reset(from_stream->Clone(0, 0, from_stream->GetWidth(), from_stream->GetHeight(), PixelFormat32bppARGB));
		}
		from_stream.reset();
		stream->Release();
		return copy;
	}

	// ------------------------------------------------------------------------------------------
	// Settings (SerenityJediEngine2027-Launcher.ini in Documents\STARWARS SerenityJediEngine\SerenityJediEngine2027,
	// with the game's configs and saves: a game installed under Program Files can't write next to its exe)
	// ------------------------------------------------------------------------------------------

	std::wstring IniPath()
	{
		static std::wstring path;
		if (path.empty())
		{
			wchar_t docs[MAX_PATH] = {};
			if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, 0, docs)))
			{
				std::wstring dir = std::wstring(docs) + L"\\" + kHomeParent;
				CreateDirectoryW(dir.c_str(), nullptr);
				dir += L"\\" + std::wstring(kHomeFolder);
				CreateDirectoryW(dir.c_str(), nullptr);
				path = dir + L"\\" + kIniName;

				// the first time, take over the options from where older launchers kept them: My Games, or next to
				// the launcher
				const std::wstring old_inis[] = {
					std::wstring(docs) + L"\\My Games\\" + kHomeFolder + L"\\" + kIniName,
					g_launcher.folder + L"\\" + kIniName };
				for (const std::wstring& old_ini : old_inis)
				{
					if (!PathFileExistsW(path.c_str()) && PathFileExistsW(old_ini.c_str()))
					{
						CopyFileW(old_ini.c_str(), path.c_str(), TRUE);
					}
				}
			}
			else
			{
				path = g_launcher.folder + L"\\" + kIniName; // no Documents folder: next to the launcher
			}
		}
		return path;
	}

	void LoadGameConfig(); // below, with the other game config code

	void LoadSettings()
	{
		const std::wstring ini = IniPath();
		g_launcher.settings.rend2 = GetPrivateProfileIntW(L"Options", L"Rend2", 0, ini.c_str()) != 0;
		g_launcher.settings.windowed = GetPrivateProfileIntW(L"Options", L"Windowed", 0, ini.c_str()) != 0;
		g_launcher.settings.controller = GetPrivateProfileIntW(L"Options", L"Controller", 1, ini.c_str()) != 0;
		g_launcher.settings.music = GetPrivateProfileIntW(L"Options", L"Music", 1, ini.c_str()) != 0;
		LoadGameConfig(); // renderer, display, resolution: what the game's config says
	}

	void SaveSettings()
	{
		const std::wstring ini = IniPath();
		WritePrivateProfileStringW(L"Options", L"Rend2", g_launcher.settings.rend2 ? L"1" : L"0", ini.c_str());
		WritePrivateProfileStringW(L"Options", L"Windowed", g_launcher.settings.windowed ? L"1" : L"0", ini.c_str());
		WritePrivateProfileStringW(L"Options", L"Controller", g_launcher.settings.controller ? L"1" : L"0", ini.c_str());
		WritePrivateProfileStringW(L"Options", L"Music", g_launcher.settings.music ? L"1" : L"0", ini.c_str());
	}

	// ------------------------------------------------------------------------------------------
	// The game's own configs: renderer, display and resolution are read from the config the game uses (the one in
	// Documents\STARWARS SerenityJediEngine\SerenityJediEngine2027\SerenityJediEngine2027 when it exists, else the one in
	// the install's SerenityJediEngine2027 folder) and written back into both the SP and the MP config: only those
	// "seta" lines are replaced (or added), every other line stays as it was.
	// ------------------------------------------------------------------------------------------

	std::wstring HomeModFolder()
	{
		wchar_t docs[MAX_PATH] = {};
		if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, 0, docs)))
		{
			return L"";
		}
		return std::wstring(docs) + L"\\" + kHomeParent + L"\\" + kHomeFolder + L"\\" + kModFolder;
	}

	// the config file of SP or MP: Documents first, then the install folder; when neither exists, the Documents one
	// (written on save; the game reads it when it starts)
	std::wstring ConfigPath(const bool multiplayer)
	{
		const wchar_t* const name = multiplayer ? kMPConfig : kSPConfig;
		const std::wstring home_dir = HomeModFolder();
		const std::wstring home = home_dir.empty() ? L"" : home_dir + L"\\" + name;
		const std::wstring install = g_launcher.folder + L"\\" + kModFolder + L"\\" + name;
		if (!home.empty() && PathFileExistsW(home.c_str()))
		{
			return home;
		}
		if (PathFileExistsW(install.c_str()))
		{
			return install;
		}
		return home.empty() ? install : home;
	}

	std::vector<std::string> ReadLines(const std::wstring& path)
	{
		std::vector<std::string> lines;
		FILE* f = nullptr;
		if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f)
		{
			return lines;
		}
		std::string text;
		char buf[4096];
		size_t n;
		while ((n = fread(buf, 1, sizeof buf, f)) > 0)
		{
			text.append(buf, n);
		}
		fclose(f);
		size_t start = 0;
		while (start < text.size())
		{
			size_t end = text.find('\n', start);
			if (end == std::string::npos)
			{
				end = text.size();
			}
			std::string line = text.substr(start, end - start);
			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}
			lines.push_back(line);
			start = end + 1;
		}
		return lines;
	}

	// the cvar name of a "seta name value" / "set name value" line, else ""
	std::string CfgLineCvar(const std::string& line, std::string* value)
	{
		size_t i = line.find_first_not_of(" \t");
		if (i == std::string::npos)
		{
			return "";
		}
		size_t word_end = line.find_first_of(" \t", i);
		if (word_end == std::string::npos)
		{
			return "";
		}
		const std::string cmd = line.substr(i, word_end - i);
		if (_stricmp(cmd.c_str(), "seta") && _stricmp(cmd.c_str(), "set"))
		{
			return "";
		}
		i = line.find_first_not_of(" \t", word_end);
		if (i == std::string::npos)
		{
			return "";
		}
		word_end = line.find_first_of(" \t", i);
		const std::string name = line.substr(i, word_end == std::string::npos ? std::string::npos : word_end - i);
		if (value)
		{
			value->clear();
			if (word_end != std::string::npos)
			{
				std::string v = line.substr(line.find_first_not_of(" \t", word_end) == std::string::npos ? line.size()
					: line.find_first_not_of(" \t", word_end));
				if (v.size() >= 2 && v.front() == '"')
				{
					const size_t close = v.find('"', 1);
					v = v.substr(1, close == std::string::npos ? std::string::npos : close - 1);
				}
				*value = v;
			}
		}
		return name;
	}

	bool ReadCfgValue(const std::vector<std::string>& lines, const char* cvar, std::string& value)
	{
		bool found = false;
		for (const std::string& line : lines)
		{
			std::string v;
			if (!_stricmp(CfgLineCvar(line, &v).c_str(), cvar))
			{
				value = v; // the last one wins, as when the game execs it
				found = true;
			}
		}
		return found;
	}

	// replaces the lines of these cvars (or adds them at the end); the rest of the file is kept as it was
	void WriteCfgValues(const std::wstring& path, const std::vector<std::pair<std::string, std::string>>& values)
	{
		std::vector<std::string> lines = ReadLines(path);
		for (const auto& kv : values)
		{
			const std::string line = "seta " + kv.first + " \"" + kv.second + "\"";
			bool replaced = false;
			for (std::string& l : lines)
			{
				if (!_stricmp(CfgLineCvar(l, nullptr).c_str(), kv.first.c_str()))
				{
					l = line;
					replaced = true;
				}
			}
			if (!replaced)
			{
				// keep a last empty line (the file's final newline) last
				if (!lines.empty() && lines.back().empty())
				{
					lines.insert(lines.end() - 1, line);
				}
				else
				{
					lines.push_back(line);
				}
			}
		}
		const size_t slash = path.find_last_of(L"\\/");
		if (slash != std::wstring::npos)
		{
			SHCreateDirectoryExW(nullptr, path.substr(0, slash).c_str(), nullptr);
		}
		FILE* f = nullptr;
		if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || !f)
		{
			return;
		}
		for (size_t i = 0; i < lines.size(); i++)
		{
			fputs(lines[i].c_str(), f);
			if (i + 1 < lines.size() || !lines[i].empty())
			{
				fputs("\r\n", f);
			}
		}
		fclose(f);
	}

	// renderer, display and resolution from the game's config (MP's, else SP's); keeps the launcher's values when
	// there is no config yet
	void LoadGameConfig()
	{
		Settings& s = g_launcher.settings;
		std::vector<std::string> lines = ReadLines(ConfigPath(true));
		if (lines.empty())
		{
			lines = ReadLines(ConfigPath(false));
		}
		if (lines.empty())
		{
			return;
		}
		std::string v;
		if (ReadCfgValue(lines, "cl_renderer", v))
		{
			s.rend2 = v.find("rend2") != std::string::npos;
		}
		if (ReadCfgValue(lines, "r_fullscreen", v))
		{
			s.windowed = atoi(v.c_str()) == 0;
		}
		if (ReadCfgValue(lines, "r_mode", v))
		{
			const int mode = atoi(v.c_str());
			if (mode == -1)
			{
				std::string w, h;
				ReadCfgValue(lines, "r_customwidth", w);
				ReadCfgValue(lines, "r_customheight", h);
				s.resolution = { atoi(w.c_str()), atoi(h.c_str()) };
			}
			else if (mode >= 0 && mode < static_cast<int>(sizeof kRModes / sizeof kRModes[0]))
			{
				s.resolution = kRModes[mode];
			}
			else
			{
				s.resolution = { 0, 0 };
			}
			if (s.resolution.width <= 0 || s.resolution.height <= 0)
			{
				s.resolution = { 0, 0 };
			}
		}
	}

	const char* RendererName(const bool rend2, const bool multiplayer)
	{
		return multiplayer
			? (rend2 ? "SerenityJediEngine2027-rdmp-rend2" : "SerenityJediEngine2027-rdmp")
			: (rend2 ? "SerenityJediEngine2027-rdsp-rend2" : "SerenityJediEngine2027-rdsp");
	}

	// the cvars the OPTIONS page sets, for the SP or the MP config / command line
	std::vector<std::pair<std::string, std::string>> GameConfigValues(const bool multiplayer)
	{
		const Settings& s = g_launcher.settings;
		std::vector<std::pair<std::string, std::string>> values = {
			{ "cl_renderer", RendererName(s.rend2, multiplayer) },
			{ "com_rend2", s.rend2 ? "1" : "0" },
			{ "r_fullscreen", s.windowed ? "0" : "1" } };
		if (s.resolution.width > 0)
		{
			values.push_back({ "r_mode", "-1" });
			values.push_back({ "r_customwidth", std::to_string(s.resolution.width) });
			values.push_back({ "r_customheight", std::to_string(s.resolution.height) });
		}
		else
		{
			values.push_back({ "r_mode", "-2" });
		}
		return values;
	}

	void SaveGameConfig()
	{
		WriteCfgValues(ConfigPath(false), GameConfigValues(false));
		WriteCfgValues(ConfigPath(true), GameConfigValues(true));
	}

	// the next resolution of the list (from a resolution that is not in it: the first)
	void NextResolution()
	{
		Settings& s = g_launcher.settings;
		const int count = static_cast<int>(sizeof kResolutions / sizeof kResolutions[0]);
		int next = 0;
		for (int i = 0; i < count; i++)
		{
			if (kResolutions[i].width == s.resolution.width && kResolutions[i].height == s.resolution.height)
			{
				next = (i + 1) % count;
				break;
			}
		}
		s.resolution = kResolutions[next];
	}

	// ------------------------------------------------------------------------------------------
	// Layout
	// ------------------------------------------------------------------------------------------

	// the buttons on the current page: the multiplayer list has three, the others four
	int ButtonCount()
	{
		if (g_launcher.page == Page::Options)
		{
			return kButtonCount + 1; // renderer, display, resolution, controller, back
		}
		return g_launcher.page == Page::Multiplayer ? 3 : kButtonCount;
	}

	RECT ButtonRect(const int index)
	{
		const int x = (kWidth - kButtonW) / 2;
		// a page with more than kButtonCount buttons starts higher, so the last one stays in the window
		const int extra = ButtonCount() > kButtonCount ? ButtonCount() - kButtonCount : 0;
		const int y = kButtonTop + (index - extra) * (kButtonH + kButtonGap);
		return RECT{ x, y, x + kButtonW, y + kButtonH };
	}

	RECT MusicRect() { return RECT{ kWidth - 84, 8, kWidth - 64, 28 }; }
	RECT MinimiseRect() { return RECT{ kWidth - 58, 8, kWidth - 38, 28 }; }
	RECT CloseRect() { return RECT{ kWidth - 32, 8, kWidth - 12, 28 }; }

	int HitTest(const POINT pt)
	{
		for (int i = 0; i < ButtonCount(); i++)
		{
			const RECT r = ButtonRect(i);
			if (PtInRect(&r, pt))
			{
				return i;
			}
		}
		const RECT music_rect = MusicRect();
		if (PtInRect(&music_rect, pt))
		{
			return kHitMusic;
		}
		const RECT min_rect = MinimiseRect();
		if (PtInRect(&min_rect, pt))
		{
			return kHitMinimise;
		}
		const RECT close_rect = CloseRect();
		if (PtInRect(&close_rect, pt))
		{
			return kHitClose;
		}
		return kHitNone;
	}

	std::wstring ButtonLabel(const int index)
	{
		if (g_launcher.page == Page::Main)
		{
			static const wchar_t* const labels[kButtonCount] = { L"SINGLEPLAYER", L"MULTIPLAYER", L"OPTIONS", L"EXIT" };
			return labels[index];
		}
		if (g_launcher.page == Page::Multiplayer)
		{
			static const wchar_t* const mp_labels[3] = { L"START MULTIPLAYER", L"START SERVER", L"BACK" };
			return mp_labels[index];
		}
		const Settings& s = g_launcher.settings;
		switch (index)
		{
		case 0: return s.rend2 ? L"GRAPHIC MODE: QUALITY" : L"GRAPHIC MODE: PERFORMANCE";
		case 1: return s.windowed ? L"DISPLAY: WINDOWED" : L"DISPLAY: FULLSCREEN";
		case 2:
			return s.resolution.width > 0
				? L"RESOLUTION: " + std::to_wstring(s.resolution.width) + L" x " + std::to_wstring(s.resolution.height)
				: L"RESOLUTION: DESKTOP";
		case 3: return s.controller ? L"CONTROLLER: ON" : L"CONTROLLER: OFF";
		default: return L"BACK";
		}
	}

	// the build date, the same as the game's (win32/AutoVersion.h)
	const wchar_t* VersionText()
	{
		return L"" VERSION_STRING;
	}

	// ------------------------------------------------------------------------------------------
	// Drawing
	// ------------------------------------------------------------------------------------------

	void AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& r, const Gdiplus::REAL radius)
	{
		const Gdiplus::REAL d = radius * 2;
		path.AddArc(r.X, r.Y, d, d, 180, 90);
		path.AddArc(r.X + r.Width - d, r.Y, d, d, 270, 90);
		path.AddArc(r.X + r.Width - d, r.Y + r.Height - d, d, d, 0, 90);
		path.AddArc(r.X, r.Y + r.Height - d, d, d, 90, 90);
		path.CloseFigure();
	}

	void DrawButton(Gdiplus::Graphics& g, const int index, const Gdiplus::Font& font)
	{
		const bool lit = g_launcher.hover == index;
		const bool down = lit && g_launcher.pressed == index;

		const RECT r = ButtonRect(index);
		const int shift = down ? 1 : 0;
		const Gdiplus::RectF box(static_cast<Gdiplus::REAL>(r.left) + 0.5f, static_cast<Gdiplus::REAL>(r.top + shift) + 0.5f,
			static_cast<Gdiplus::REAL>(kButtonW) - 1.0f, static_cast<Gdiplus::REAL>(kButtonH) - 1.0f);
		Gdiplus::GraphicsPath path;
		AddRoundRect(path, box, 7.0f);

		// dark glassy bar; on hover a blue glow around it, like a lit saber
		if (lit)
		{
			for (const Gdiplus::REAL width : { 9.0f, 5.0f, 2.5f })
			{
				Gdiplus::Pen glow(kGlow, width);
				g.DrawPath(&glow, &path);
			}
		}
		const Gdiplus::LinearGradientBrush fill(Gdiplus::RectF(box.X, box.Y - 1, box.Width, box.Height + 2),
			lit ? Gdiplus::Color(225, 22, 62, 122) : Gdiplus::Color(200, 10, 18, 34),
			lit ? Gdiplus::Color(225, 6, 20, 48) : Gdiplus::Color(200, 2, 6, 14),
			Gdiplus::LinearGradientModeVertical);
		g.FillPath(&fill, &path);
		Gdiplus::Pen edge(lit ? kEdgeLit : kEdgeColor, lit ? 1.5f : 1.0f);
		g.DrawPath(&edge, &path);

		Gdiplus::StringFormat fmt;
		fmt.SetAlignment(Gdiplus::StringAlignmentCenter);
		fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);

		const std::wstring label = ButtonLabel(index);
		const Gdiplus::RectF text_rect(box.X, box.Y, box.Width, box.Height);
		const Gdiplus::RectF shadow_rect(text_rect.X + 1, text_rect.Y + 2, text_rect.Width, text_rect.Height);

		// long labels shrink to stay inside the bar
		const Gdiplus::REAL max_width = static_cast<Gdiplus::REAL>(kButtonW) - 40.0f;
		Gdiplus::RectF bounds;
		g.MeasureString(label.c_str(), -1, &font, Gdiplus::PointF(0, 0), &bounds);
		std::unique_ptr<Gdiplus::Font> fitted;
		const Gdiplus::Font* use_font = &font;
		if (bounds.Width > max_width)
		{
			Gdiplus::FontFamily family;
			font.GetFamily(&family);
			fitted = std::make_unique<Gdiplus::Font>(&family, font.GetSize() * max_width / bounds.Width, font.GetStyle(), font.GetUnit());
			use_font = fitted.get();
		}

		const Gdiplus::SolidBrush shadow(Gdiplus::Color(200, 0, 0, 0));
		const Gdiplus::SolidBrush text(lit ? kTextLit : kTextColor);
		g.DrawString(label.c_str(), -1, use_font, shadow_rect, &fmt, &shadow);
		g.DrawString(label.c_str(), -1, use_font, text_rect, &fmt, &text);
	}

	void DrawWindowButtons(Gdiplus::Graphics& g)
	{
		const Gdiplus::Color blue(220, 90, 150, 220);
		const Gdiplus::Color bright(255, 150, 210, 255);

		// speaker: waves when the music is on, a cross when it is off
		const RECT s = MusicRect();
		Gdiplus::Pen music_pen(g_launcher.hover == kHitMusic ? bright : blue, 2);
		const Gdiplus::REAL sx = static_cast<Gdiplus::REAL>(s.left);
		const Gdiplus::REAL sy = static_cast<Gdiplus::REAL>(s.top);
		g.DrawEllipse(&music_pen, sx, sy, 20.0f, 20.0f);
		const Gdiplus::PointF speaker[] = {
			{ sx + 4.5f, sy + 8.0f },{ sx + 7.5f, sy + 8.0f },{ sx + 11.0f, sy + 5.0f },
			{ sx + 11.0f, sy + 15.0f },{ sx + 7.5f, sy + 12.0f },{ sx + 4.5f, sy + 12.0f } };
		Gdiplus::Pen thin_pen(g_launcher.hover == kHitMusic ? bright : blue, 1.5f);
		g.DrawPolygon(&thin_pen, speaker, 6);
		if (g_launcher.settings.music)
		{
			g.DrawArc(&thin_pen, sx + 9.0f, sy + 7.0f, 6.0f, 6.0f, -60.0f, 120.0f);
			g.DrawArc(&thin_pen, sx + 8.0f, sy + 4.5f, 9.5f, 11.0f, -60.0f, 120.0f);
		}
		else
		{
			g.DrawLine(&thin_pen, sx + 13.0f, sy + 8.0f, sx + 17.0f, sy + 12.0f);
			g.DrawLine(&thin_pen, sx + 17.0f, sy + 8.0f, sx + 13.0f, sy + 12.0f);
		}

		const RECT m = MinimiseRect();
		Gdiplus::Pen min_pen(g_launcher.hover == kHitMinimise ? bright : blue, 2);
		g.DrawEllipse(&min_pen, static_cast<INT>(m.left), static_cast<INT>(m.top), 20, 20);
		g.DrawLine(&min_pen, static_cast<INT>(m.left + 5), static_cast<INT>(m.top + 10), static_cast<INT>(m.left + 15), static_cast<INT>(m.top + 10));

		const RECT c = CloseRect();
		Gdiplus::Pen close_pen(g_launcher.hover == kHitClose ? bright : blue, 2);
		g.DrawEllipse(&close_pen, static_cast<INT>(c.left), static_cast<INT>(c.top), 20, 20);
		g.DrawLine(&close_pen, static_cast<INT>(c.left + 5), static_cast<INT>(c.top + 5), static_cast<INT>(c.left + 15), static_cast<INT>(c.top + 15));
		g.DrawLine(&close_pen, static_cast<INT>(c.left + 15), static_cast<INT>(c.top + 5), static_cast<INT>(c.left + 5), static_cast<INT>(c.top + 15));
	}

	void Paint(const HWND hwnd)
	{
		PAINTSTRUCT ps;
		const HDC hdc = BeginPaint(hwnd, &ps);

		// draw into a back buffer, then copy it in one go (no flicker)
		Gdiplus::Bitmap buffer(kWidth, kHeight, PixelFormat32bppARGB);
		{
			Gdiplus::Graphics g(&buffer);
			g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
			g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
			g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

			if (g_launcher.background)
			{
				g.DrawImage(g_launcher.background.get(), 0, 0, kWidth, kHeight);
			}
			else
			{
				g.Clear(Gdiplus::Color(255, 0, 0, 0));
			}

			// a Trajan-like serif as on the SJE icon lettering; Palatino Linotype ships with Windows
			Gdiplus::FontFamily family(L"Palatino Linotype");
			const Gdiplus::FontFamily* use_family = family.IsAvailable() ? &family : Gdiplus::FontFamily::GenericSerif();
			const Gdiplus::Font font(use_family, 14, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
			for (int i = 0; i < ButtonCount(); i++)
			{
				DrawButton(g, i, font);
			}

			DrawWindowButtons(g);

			Gdiplus::FontFamily ui_family(L"Segoe UI");
			const Gdiplus::Font version_font(ui_family.IsAvailable() ? &ui_family : Gdiplus::FontFamily::GenericSansSerif(), 8, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
			const Gdiplus::SolidBrush grey(Gdiplus::Color(160, 200, 200, 200));
			Gdiplus::StringFormat right;
			right.SetAlignment(Gdiplus::StringAlignmentFar);
			g.DrawString(VersionText(), -1, &version_font, Gdiplus::RectF(0, kHeight - 20, kWidth - 10, 20), &right, &grey);
		}

		// copy the finished frame 1:1, no filtering
		Gdiplus::Graphics screen(hdc);
		screen.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
		screen.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
		screen.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
		screen.DrawImage(&buffer, 0, 0, kWidth, kHeight);
		EndPaint(hwnd, &ps);
	}

	// ------------------------------------------------------------------------------------------
	// Starting the game
	// ------------------------------------------------------------------------------------------

	bool StartGame(const HWND hwnd, const bool multiplayer)
	{
		const Settings& s = g_launcher.settings;
		const std::wstring exe = g_launcher.folder + L"\\" + (multiplayer ? kMPExe : kSPExe);
		if (!PathFileExistsW(exe.c_str()))
		{
			const std::wstring msg = L"Could not find " + exe + L"\n\nThe launcher must be in the SerenityJediEngine2027 game folder.";
			MessageBoxW(hwnd, msg.c_str(), L"SerenityJediEngine2027", MB_OK | MB_ICONERROR);
			return false;
		}

		// "+set" on the command line overrides the values in the configs (they are the configs' own values unless the
		// OPTIONS page changed them, and then it wrote them into the configs too)
		std::wstring cmd = L"\"" + exe + L"\"";
		for (const auto& kv : GameConfigValues(multiplayer))
		{
			cmd += L" +set " + std::wstring(kv.first.begin(), kv.first.end()) + L" " + std::wstring(kv.second.begin(), kv.second.end());
		}
		cmd += s.controller ? L" +set in_joystick 1" : L" +set in_joystick 0";

		SetEnvironmentVariableW(kLauncherEnv, kLauncherToken); // the game inherits it: lets a release build start

		STARTUPINFOW si = { sizeof(si) };
		PROCESS_INFORMATION pi = {};
		std::wstring cmd_buffer = cmd; // CreateProcessW may write into it
		if (!CreateProcessW(exe.c_str(), &cmd_buffer[0], nullptr, nullptr, FALSE, 0, nullptr, g_launcher.folder.c_str(), &si, &pi))
		{
			MessageBoxW(hwnd, (L"Could not start " + exe).c_str(), L"SerenityJediEngine2027", MB_OK | MB_ICONERROR);
			return false;
		}
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return true;
	}

	// Runs the FFA dedicated server batch file from the mod folder in a console window of its own. The batch file
	// restarts the server when it crashes; closing its window stops it.
	bool StartServer(const HWND hwnd)
	{
		const std::wstring mod = g_launcher.folder + L"\\" + kModFolder;
		const std::wstring bat = mod + L"\\" + kServerBat;
		if (!PathFileExistsW(bat.c_str()))
		{
			const std::wstring msg = L"Could not find " + bat + L"\n\nThe launcher must be in the SerenityJediEngine2027 game folder.";
			MessageBoxW(hwnd, msg.c_str(), L"SerenityJediEngine2027", MB_OK | MB_ICONERROR);
			return false;
		}

		wchar_t comspec[MAX_PATH] = {};
		if (!GetEnvironmentVariableW(L"ComSpec", comspec, MAX_PATH))
		{
			GetSystemDirectoryW(comspec, MAX_PATH);
			wcscat_s(comspec, L"\\cmd.exe");
		}
		// cmd /c ""<bat>"": the outer quotes are cmd's, the inner ones keep the spaces in the path
		std::wstring cmd = L"\"" + std::wstring(comspec) + L"\" /c \"\"" + bat + L"\"\"";

		SetEnvironmentVariableW(kLauncherEnv, kLauncherToken); // the server inherits it, as the game does

		STARTUPINFOW si = { sizeof(si) };
		PROCESS_INFORMATION pi = {};
		if (!CreateProcessW(comspec, &cmd[0], nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, mod.c_str(), &si, &pi))
		{
			MessageBoxW(hwnd, (L"Could not start " + bat).c_str(), L"SerenityJediEngine2027", MB_OK | MB_ICONERROR);
			return false;
		}
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return true;
	}

	void OnButton(const HWND hwnd, const int index)
	{
		if (g_launcher.page == Page::Main)
		{
			switch (index)
			{
			case 0:
				PlaySfx(true); // the launcher closes right after
				if (StartGame(hwnd, false))
				{
					DestroyWindow(hwnd);
				}
				return;
			case 1:
				PlaySfx();
				g_launcher.page = Page::Multiplayer;
				g_launcher.hover = kHitNone;
				break;
			case 2:
				PlaySfx();
				g_launcher.page = Page::Options;
				break;
			default:
				PlaySfx(true);
				DestroyWindow(hwnd);
				return;
			}
		}
		else if (g_launcher.page == Page::Multiplayer)
		{
			switch (index)
			{
			case 0:
				PlaySfx(true); // the launcher closes right after
				if (StartGame(hwnd, true))
				{
					DestroyWindow(hwnd);
				}
				return;
			case 1:
				PlaySfx();
				StartServer(hwnd); // the launcher stays open, to start the game and join
				break;
			default:
				PlaySfx();
				g_launcher.page = Page::Main;
				break;
			}
		}
		else
		{
			Settings& s = g_launcher.settings;
			PlaySfx();
			switch (index)
			{
			case 0: s.rend2 = !s.rend2; break;
			case 1: s.windowed = !s.windowed; break;
			case 2: NextResolution(); break;
			case 3: s.controller = !s.controller; break;
			default: g_launcher.page = Page::Main; break;
			}
			SaveSettings();
			if (index <= 2)
			{
				SaveGameConfig(); // renderer, display, resolution: into the game's configs
			}
		}
		InvalidateRect(hwnd, nullptr, FALSE);
	}

	// ------------------------------------------------------------------------------------------
	// Window
	// ------------------------------------------------------------------------------------------

	void SetHover(const HWND hwnd, const int hit)
	{
		if (hit != g_launcher.hover)
		{
			g_launcher.hover = hit;
			InvalidateRect(hwnd, nullptr, FALSE);
		}
	}

	LRESULT CALLBACK WndProc(const HWND hwnd, const UINT msg, const WPARAM wparam, const LPARAM lparam)
	{
		switch (msg)
		{
		case WM_NCHITTEST:
		{
			// drag the window by anything that is not a button
			POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
			ScreenToClient(hwnd, &pt);
			return HitTest(pt) == kHitNone ? HTCAPTION : HTCLIENT;
		}
		case WM_MOUSEMOVE:
		{
			if (!g_launcher.tracking_mouse)
			{
				TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
				g_launcher.tracking_mouse = TrackMouseEvent(&tme) != FALSE;
			}
			const int hit = HitTest(POINT{ GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) });
			if (hit != g_launcher.hover && hit >= 0 && hit < ButtonCount())
			{
				PlaySfx(); // moved onto another menu button
			}
			SetHover(hwnd, hit);
			return 0;
		}
		case WM_MOUSELEAVE:
		case WM_NCMOUSEMOVE:
			g_launcher.tracking_mouse = false;
			g_launcher.pressed = kHitNone;
			SetHover(hwnd, kHitNone);
			return 0;
		case WM_LBUTTONDOWN:
			g_launcher.pressed = HitTest(POINT{ GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) });
			SetCapture(hwnd);
			InvalidateRect(hwnd, nullptr, FALSE);
			return 0;
		case WM_LBUTTONUP:
		{
			ReleaseCapture();
			const int hit = HitTest(POINT{ GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) });
			const int pressed = g_launcher.pressed;
			g_launcher.pressed = kHitNone;
			InvalidateRect(hwnd, nullptr, FALSE);
			if (hit != pressed || hit == kHitNone)
			{
				return 0; // released somewhere else
			}
			if (hit == kHitClose)
			{
				PlaySfx(true);
				DestroyWindow(hwnd);
			}
			else if (hit == kHitMinimise)
			{
				PlaySfx();
				ShowWindow(hwnd, SW_MINIMIZE);
			}
			else if (hit == kHitMusic)
			{
				PlaySfx();
				g_launcher.settings.music = !g_launcher.settings.music;
				SaveSettings();
				if (g_launcher.settings.music)
				{
					StartMusic(hwnd);
				}
				else
				{
					StopMusic();
				}
			}
			else
			{
				OnButton(hwnd, hit);
			}
			return 0;
		}
		case WM_KEYDOWN:
			if (wparam == VK_ESCAPE)
			{
				if (g_launcher.page != Page::Main)
				{
					PlaySfx();
					g_launcher.page = Page::Main;
					InvalidateRect(hwnd, nullptr, FALSE);
				}
				else
				{
					PlaySfx(true);
					DestroyWindow(hwnd);
				}
			}
			return 0;
		case MM_MCINOTIFY:
			// the music reached its end: play it again (loop). The menu sound plays without notify.
			if (wparam == MCI_NOTIFY_SUCCESSFUL && g_launcher.music_open)
			{
				PlayMusicFromStart(hwnd);
			}
			return 0;
		case WM_ERASEBKGND:
			return 1; // everything is painted in WM_PAINT
		case WM_PAINT:
			Paint(hwnd);
			return 0;
		case WM_DESTROY:
			StopMusic();
			PostQuitMessage(0);
			return 0;
		default:
			return DefWindowProcW(hwnd, msg, wparam, lparam);
		}
	}
}

int WINAPI wWinMain(_In_ const HINSTANCE instance, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ const int show) // SAL as winbase.h (C28251)
{
	g_launcher.instance = instance;

	wchar_t path[MAX_PATH] = {};
	GetModuleFileNameW(nullptr, path, MAX_PATH);
	PathRemoveFileSpecW(path);
	g_launcher.folder = path;
	LoadSettings();

	Gdiplus::GdiplusStartupInput gdiplus_input;
	ULONG_PTR gdiplus_token = 0;
	Gdiplus::GdiplusStartup(&gdiplus_token, &gdiplus_input, nullptr);
	g_launcher.background = LoadPng(IDR_BG);

	WNDCLASSEXW wc = { sizeof(wc) };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = instance;
	wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_LAUNCHER));
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.lpszClassName = L"SerenityJediEngine2027Launcher";
	RegisterClassExW(&wc);

	// centred, borderless (the art has its own frame and window buttons)
	const int x = (GetSystemMetrics(SM_CXSCREEN) - kWidth) / 2;
	const int y = (GetSystemMetrics(SM_CYSCREEN) - kHeight) / 2;
	const HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"SerenityJediEngine2027", WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU,
		x, y, kWidth, kHeight, nullptr, nullptr, instance, nullptr);
	if (!hwnd)
	{
		g_launcher.background.reset();
		Gdiplus::GdiplusShutdown(gdiplus_token);
		return 1;
	}
	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);
	StartMusic(hwnd);

	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	DeleteTempFile(g_launcher.music_file);
	g_launcher.background.reset(); // before GDI+ goes
	Gdiplus::GdiplusShutdown(gdiplus_token);
	return 0;
}