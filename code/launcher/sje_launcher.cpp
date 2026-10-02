/*
===========================================================================
SerenityJediEngine2026 launcher
===========================================================================

A small window like the Jedi Academy launcher: SINGLEPLAYER, MULTIPLAYER, OPTIONS and EXIT.
It starts SerenityJediEngine2026-SP.x86_64.exe or SerenityJediEngine2026-MP.x86_64.exe from its own
folder, passing the options chosen on the OPTIONS page as "+set" command line values (they override
the configs), and then closes. The options are remembered in SerenityJediEngine2026-Launcher.ini in
Documents\My Games\SerenityJediEngine2026, next to the game's own configs.

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

	const wchar_t* const kSPExe = L"SerenityJediEngine2026-SP.x86_64.exe";
	const wchar_t* const kMPExe = L"SerenityJediEngine2026-MP.x86_64.exe";
	const wchar_t* const kIniName = L"SerenityJediEngine2026-Launcher.ini";
	const wchar_t* const kHomeFolder = L"SerenityJediEngine2026"; // the game's folder in My Games (HOMEPATH_NAME_WIN)

	// SJE colours: the blue of the menu art and the icons
	const Gdiplus::Color kTextColor(255, 177, 194, 220);  // the main menu's button text (.695 .760 .861)
	const Gdiplus::Color kTextLit(255, 235, 245, 255);
	const Gdiplus::Color kEdgeColor(150, 70, 130, 200);
	const Gdiplus::Color kEdgeLit(235, 120, 200, 255);
	const Gdiplus::Color kGlow(45, 80, 170, 255);

	enum class Page { Main, Options };

	struct Settings
	{
		bool rend2 = false;
		bool windowed = false;
		bool controller = true; // in_joystick: gamepad / joystick input (the game default is on)
		bool music = true;  // launcher background music (speaker button)
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
			g_launcher.music_file = WriteResourceToTemp(IDR_MUSIC, L"SerenityJediEngine2026-Launcher-music.mp3");
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
	// Settings (SerenityJediEngine2026-Launcher.ini in Documents\My Games\SerenityJediEngine2026, with
	// the game's configs and saves: a game installed under Program Files can't write next to its exe)
	// ------------------------------------------------------------------------------------------

	std::wstring IniPath()
	{
		static std::wstring path;
		if (path.empty())
		{
			wchar_t docs[MAX_PATH] = {};
			if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, 0, docs)))
			{
				std::wstring dir = std::wstring(docs) + L"\\My Games";
				CreateDirectoryW(dir.c_str(), nullptr);
				dir += L"\\" + std::wstring(kHomeFolder);
				CreateDirectoryW(dir.c_str(), nullptr);
				path = dir + L"\\" + kIniName;

				// the first time, take over the options from an ini next to the launcher (older launchers)
				const std::wstring old_ini = g_launcher.folder + L"\\" + kIniName;
				if (!PathFileExistsW(path.c_str()) && PathFileExistsW(old_ini.c_str()))
				{
					CopyFileW(old_ini.c_str(), path.c_str(), TRUE);
				}
			}
			else
			{
				path = g_launcher.folder + L"\\" + kIniName; // no Documents folder: next to the launcher
			}
		}
		return path;
	}

	void LoadSettings()
	{
		const std::wstring ini = IniPath();
		g_launcher.settings.rend2 = GetPrivateProfileIntW(L"Options", L"Rend2", 0, ini.c_str()) != 0;
		g_launcher.settings.windowed = GetPrivateProfileIntW(L"Options", L"Windowed", 0, ini.c_str()) != 0;
		g_launcher.settings.controller = GetPrivateProfileIntW(L"Options", L"Controller", 1, ini.c_str()) != 0;
		g_launcher.settings.music = GetPrivateProfileIntW(L"Options", L"Music", 1, ini.c_str()) != 0;
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
	// Layout
	// ------------------------------------------------------------------------------------------

	RECT ButtonRect(const int index)
	{
		const int x = (kWidth - kButtonW) / 2;
		const int y = kButtonTop + index * (kButtonH + kButtonGap);
		return RECT{ x, y, x + kButtonW, y + kButtonH };
	}

	RECT MusicRect() { return RECT{ kWidth - 84, 8, kWidth - 64, 28 }; }
	RECT MinimiseRect() { return RECT{ kWidth - 58, 8, kWidth - 38, 28 }; }
	RECT CloseRect() { return RECT{ kWidth - 32, 8, kWidth - 12, 28 }; }

	int HitTest(const POINT pt)
	{
		for (int i = 0; i < kButtonCount; i++)
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
		const Settings& s = g_launcher.settings;
		switch (index)
		{
		case 0: return s.rend2 ? L"RENDERER: REND2" : L"RENDERER: VANILLA";
		case 1: return s.windowed ? L"DISPLAY: WINDOWED" : L"DISPLAY: FULLSCREEN";
		case 2: return s.controller ? L"CONTROLLER: ON" : L"CONTROLLER: OFF";
		default: return L"BACK";
		}
	}

	std::wstring VersionText()
	{
		wchar_t text[32];
		swprintf_s(text, L"Build %02d", VERSION_INTERNAL_BUILD);
		return text;
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
			{ sx + 4.5f, sy + 8.0f }, { sx + 7.5f, sy + 8.0f }, { sx + 11.0f, sy + 5.0f },
			{ sx + 11.0f, sy + 15.0f }, { sx + 7.5f, sy + 12.0f }, { sx + 4.5f, sy + 12.0f } };
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
			for (int i = 0; i < kButtonCount; i++)
			{
				DrawButton(g, i, font);
			}

			DrawWindowButtons(g);

			Gdiplus::FontFamily ui_family(L"Segoe UI");
			const Gdiplus::Font version_font(ui_family.IsAvailable() ? &ui_family : Gdiplus::FontFamily::GenericSansSerif(), 8, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
			const Gdiplus::SolidBrush grey(Gdiplus::Color(160, 200, 200, 200));
			g.DrawString(VersionText().c_str(), -1, &version_font, Gdiplus::PointF(10, kHeight - 20), &grey);
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
			const std::wstring msg = L"Could not find " + exe + L"\n\nThe launcher must be in the SerenityJediEngine2026 game folder.";
			MessageBoxW(hwnd, msg.c_str(), L"SerenityJediEngine2026", MB_OK | MB_ICONERROR);
			return false;
		}

		// "+set" on the command line overrides the values in the configs
		std::wstring cmd = L"\"" + exe + L"\"";
		const wchar_t* renderer = multiplayer
			? (s.rend2 ? L"SerenityJediEngine2026-rdmp-rend2" : L"SerenityJediEngine2026-rdmp")
			: (s.rend2 ? L"SerenityJediEngine2026-rdsp-rend2" : L"SerenityJediEngine2026-rdsp");
		cmd += L" +set cl_renderer ";
		cmd += renderer;
		cmd += s.rend2 ? L" +set com_rend2 1" : L" +set com_rend2 0";
		cmd += s.windowed ? L" +set r_fullscreen 0" : L" +set r_fullscreen 1";
		cmd += s.controller ? L" +set in_joystick 1" : L" +set in_joystick 0";

		STARTUPINFOW si = { sizeof(si) };
		PROCESS_INFORMATION pi = {};
		std::wstring cmd_buffer = cmd; // CreateProcessW may write into it
		if (!CreateProcessW(exe.c_str(), &cmd_buffer[0], nullptr, nullptr, FALSE, 0, nullptr, g_launcher.folder.c_str(), &si, &pi))
		{
			MessageBoxW(hwnd, (L"Could not start " + exe).c_str(), L"SerenityJediEngine2026", MB_OK | MB_ICONERROR);
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
			case 1:
				PlaySfx(true); // the launcher closes right after
				if (StartGame(hwnd, index == 1))
				{
					DestroyWindow(hwnd);
				}
				return;
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
		else
		{
			Settings& s = g_launcher.settings;
			PlaySfx();
			switch (index)
			{
			case 0: s.rend2 = !s.rend2; break;
			case 1: s.windowed = !s.windowed; break;
			case 2: s.controller = !s.controller; break;
			default: g_launcher.page = Page::Main; break;
			}
			SaveSettings();
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
			if (hit != g_launcher.hover && hit >= 0 && hit < kButtonCount)
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
				if (g_launcher.page == Page::Options)
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

int WINAPI wWinMain(const HINSTANCE instance, HINSTANCE, PWSTR, const int show)
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
	wc.lpszClassName = L"SerenityJediEngine2026Launcher";
	RegisterClassExW(&wc);

	// centred, borderless (the art has its own frame and window buttons)
	const int x = (GetSystemMetrics(SM_CXSCREEN) - kWidth) / 2;
	const int y = (GetSystemMetrics(SM_CYSCREEN) - kHeight) / 2;
	const HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"SerenityJediEngine2026", WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU,
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