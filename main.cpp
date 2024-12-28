#define UNICODE
#define WIN32_LEAN_AND_MEAN

#include <stdio.h> // swprintf_s
#include <windows.h>

const wchar_t lpClassName[] = L"TouchStone";
const wchar_t lpWindowTitle[] = L"Touch Stone 2004";

#include "resource.h"

#include "registry.h"

#define REG_KEY_NAME L"Software\\Relative\\TouchStone" // app reg key

#define REG_VALUE_SPRITES L"Graphics_Set"
#define REG_VALUE_LVL_CUR L"Level_Current"
#define REG_VALUE_LVL_MAX L"Level_Achieved"
#define REG_VALUE_MOVES L"Stone_Moves"
#define REG_VALUE_CRC L"Checksum"

#include "levels.h"

HBITMAP hBitmap = NULL;
HBITMAP hBitmapOld = NULL;
HBITMAP hBitmapMask = NULL;
HBITMAP hBitmapMaskOld = NULL;

HDC hSprites = NULL;
HDC hSpritesMask = NULL;

BYTE sprites = 0; // graphic set 0-9
BYTE lvl_cur = 0; // <= lvl_max
BYTE lvl_max = 0; // < LVL_NUM
BYTE level[TILE_ROWS][TILE_COLS];
POINTS player;	 // position
DWORD moves = 0; // score (less is better)

static DWORD GetUserSalt()
{
	DWORD salt = 0xA6DF; // default random salt
	const DWORD Len = 200;
	TCHAR szUsername[Len + 1];
	DWORD dwLen = Len;
	if (GetUserName(szUsername, &dwLen)) // GetUserNameEx(NameUniqueId)
		for (DWORD byte = 0; byte < dwLen; ++byte)
			salt += szUsername[byte];
	return salt;
}

static void LoadLevel(HWND hWnd) // convert level data to bit data
{
	for (BYTE row = 0; row < TILE_ROWS; row++)
	{
		for (BYTE col = 0; col < TILE_COLS; col++)
		{
			level[row][col] = (BYTE)(levels[lvl_cur][row][col]);
			// convert char to number
			if (isdigit(level[row][col]))
				level[row][col] -= '0';
			else
				level[row][col] = 10 + (BYTE)toupper(level[row][col]) - 'A';
			// get player start position
			if (level[row][col] & TILE_PLAYER)
			{
				player.x = col;
				player.y = row;
			}
		}
	}
	RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

static void CheckDone(HWND hWnd)
{
	for (BYTE row = 0; row < TILE_ROWS; row++)
		for (BYTE col = 0; col < TILE_COLS; col++)
			if ((level[row][col] & TILE_HOLE) && !(level[row][col] & TILE_STONE)) // looking for empty hole
				return;															  // exit

	// level done
	lvl_cur++; // next level
	if (lvl_cur > lvl_max)
		lvl_max = lvl_cur;

	// show message
	if (lvl_cur < LVL_NUM - 1)
		MessageBox(hWnd, L"Well Done!\nGo To Next Level", lpWindowTitle, MB_OK | MB_ICONINFORMATION);
	else // last level
		MessageBox(hWnd, L"Congratulations!\nTHE END", lpWindowTitle, MB_OK | MB_ICONEXCLAMATION);

	LoadLevel(hWnd); // load new level
}

static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
	{
		// set exact window size
		RECT rcWnd, rcClient;
		GetWindowRect(hWnd, &rcWnd);
		GetClientRect(hWnd, &rcClient);
		rcWnd.right = ((rcWnd.right - rcWnd.left) - (rcClient.right - rcClient.left)) + SCENE_WIDTH;   // borders + scene width
		rcWnd.bottom = ((rcWnd.bottom - rcWnd.top) - (rcClient.bottom - rcClient.top)) + SCENE_HEIGHT; // borders + window title height + scene height
		MoveWindow(hWnd, rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom, FALSE);

		// load sprites
		hBitmap = (HBITMAP)LoadImage(NULL, L"sprites.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE); // system automatically deletes these resources
		if (!hBitmap)
		{
			MessageBox(hWnd, L"File 'sprites.bmp' not found!", lpWindowTitle, MB_OK | MB_ICONERROR);
			return -1; // window is destroyed and the CreateWindowEx or CreateWindow function returns a NULL handle
		}
		else
		{
			hSprites = CreateCompatibleDC(NULL);
			hBitmapOld = (HBITMAP)SelectObject(hSprites, hBitmap);

			// create bitmap mask
			BITMAP bitmap;
			GetObject(hBitmap, sizeof(BITMAP), &bitmap);							 // get bitmap width and height
			hBitmapMask = CreateBitmap(bitmap.bmWidth, bitmap.bmHeight, 1, 1, NULL); // create monochrome bitmap
			if (hBitmapMask)
			{
				hSpritesMask = CreateCompatibleDC(NULL);
				hBitmapMaskOld = (HBITMAP)SelectObject(hSpritesMask, hBitmapMask);

				SetBkColor(hSprites, RGB(255, 0, 255)); // transparent color (magenta)

				BitBlt(hSpritesMask, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hSprites, 0, 0, SRCCOPY);	// copy
				BitBlt(hSprites, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hSpritesMask, 0, 0, SRCINVERT); // invert
			}
		}

		// load saved data
		RegKey regkey(HKEY_CURRENT_USER, REG_KEY_NAME);
		if (regkey.IsOpen())
		{
			sprites = (BYTE)regkey.GetDword(REG_VALUE_SPRITES, sprites);
			lvl_max = (BYTE)regkey.GetDword(REG_VALUE_LVL_MAX, lvl_max);
			if (lvl_max >= LVL_NUM)
				lvl_max = LVL_NUM - 1;
			lvl_cur = (BYTE)regkey.GetDword(REG_VALUE_LVL_CUR, lvl_cur);
			if (lvl_cur > lvl_max)
				lvl_cur = lvl_max;
			moves = (BYTE)regkey.GetDword(REG_VALUE_MOVES, moves);
			if (regkey.GetDword(REG_VALUE_CRC, 0) != (moves ^ lvl_max ^ GetUserSalt()))
			{
				moves = 0;
				lvl_max = lvl_cur = 0;
				MessageBox(hWnd, L"You're a Cheater!", lpWindowTitle, MB_OK | MB_ICONEXCLAMATION);
			}
		}

		LoadLevel(hWnd); // load level data
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hWnd, &ps);
		SetBkMode(hDC, OPAQUE);

		for (BYTE row = 0; row < TILE_ROWS; row++)
		{
			for (BYTE col = 0; col < TILE_COLS; col++)
			{
				// first layer (ground & floor)
				if (level[row][col] & TILE_FLOOR)
				{
					BitBlt(hDC, col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, hSprites, SPRITE_FLOOR * TILE_SIZE, sprites * TILE_SIZE, SRCCOPY);
					// second layer (walls & holes)
					if (level[row][col] & TILE_WALL)
					{
						BitBlt(hDC, col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, hSpritesMask, SPRITE_WALL * TILE_SIZE, sprites * TILE_SIZE, SRCAND);
						BitBlt(hDC, col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, hSprites, SPRITE_WALL * TILE_SIZE, sprites * TILE_SIZE, SRCPAINT);
					}
					else
					{
						if (level[row][col] & TILE_HOLE)
						{
							BitBlt(hDC, col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, hSpritesMask, SPRITE_HOLE * TILE_SIZE, sprites * TILE_SIZE, SRCAND);
							BitBlt(hDC, col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, hSprites, SPRITE_HOLE * TILE_SIZE, sprites * TILE_SIZE, SRCPAINT);
						}
						// third layer (stones)
						if (level[row][col] & TILE_STONE)
						{
							BitBlt(hDC, col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, hSpritesMask, SPRITE_STONE * TILE_SIZE, sprites * TILE_SIZE, SRCAND);
							BitBlt(hDC, col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, hSprites, SPRITE_STONE * TILE_SIZE, sprites * TILE_SIZE, SRCPAINT);
						}
					}
				}
				else // only ground
					BitBlt(hDC, col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, hSprites, SPRITE_GROUND * TILE_SIZE, sprites * TILE_SIZE, SRCCOPY);
			}
		}
		// fourth layer (player & text)
		BitBlt(hDC, player.x * TILE_SIZE, player.y * TILE_SIZE, TILE_SIZE, TILE_SIZE, hSpritesMask, SPRITE_PLAYER * TILE_SIZE, sprites * TILE_SIZE, SRCAND);
		BitBlt(hDC, player.x * TILE_SIZE, player.y * TILE_SIZE, TILE_SIZE, TILE_SIZE, hSprites, SPRITE_PLAYER * TILE_SIZE, sprites * TILE_SIZE, SRCPAINT);
		// text
		SetBkMode(hDC, TRANSPARENT);
		COLORREF clrPrev = SetTextColor(hDC, RGB(128, 255, 255));
		wchar_t text[100];
		TextOut(hDC, 15, 15, text, swprintf_s(text, 100, L"Current level: %i / Achieved level: %i / All levels: %i", lvl_cur + 1, lvl_max + 1, LVL_NUM));
		TextOut(hDC, SCENE_WIDTH - 115, 15, text, swprintf_s(text, 100, L"Moves: %i", moves)); // score
		SetTextColor(hDC, clrPrev);
		// end painting
		EndPaint(hWnd, &ps);
		return 0; // application processes this message
	}
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_F1:
			MessageBox(hWnd, L"F1\t=> Help (this window)\nArrows\t=> Player move\n0-9\t=> Change graphic set\nPlus\t=> Next Level\nMinus\t=> Prev Level\nBack\t=> Restart Level", lpWindowTitle, MB_OK | MB_ICONINFORMATION);
			break;
		case VK_BACK:
		case VK_DIVIDE:
			LoadLevel(hWnd); // reset level
			break;
		case VK_SUBTRACT:
		case VK_OEM_MINUS:
			if (lvl_cur > 0)
			{
				lvl_cur--; // prev level
				LoadLevel(hWnd);
			}
			break;
		case VK_ADD:
		case VK_OEM_PLUS:
			if (lvl_cur < lvl_max)
			{
				lvl_cur++; // next level
				LoadLevel(hWnd);
			}
			break;
		case VK_NUMPAD0:
		case VK_NUMPAD1:
		case VK_NUMPAD2:
		case VK_NUMPAD3:
		case VK_NUMPAD4:
		case VK_NUMPAD5:
		case VK_NUMPAD6:
		case VK_NUMPAD7:
		case VK_NUMPAD8:
		case VK_NUMPAD9:
			sprites = (BYTE)wParam - VK_NUMPAD0; // change graphic set
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			break;
		case 0x41: // A
		case 0x4A: // J
		case VK_LEFT:
			if (player.x > 0 && !(level[player.y][player.x - 1] & TILE_WALL)) // allowed player move?
			{
				if (level[player.y][player.x - 1] & TILE_STONE) // next is stone?
				{
					if ((level[player.y][player.x - 2] & TILE_WALL) || (level[player.y][player.x - 2] & TILE_STONE)) // allowed stone move?
						break;																						 // skip move
					level[player.y][player.x - 1] ^= TILE_STONE;													 // stone remove
					level[player.y][player.x - 2] ^= TILE_STONE;													 // stone add
					if (lvl_cur == lvl_max)
						moves++; // score
				}
				player.x--; // player move
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
				CheckDone(hWnd);
			}
			break;
		case 0x57: // W
		case 0x49: // I
		case VK_UP:
			if (player.y > 0 && !(level[player.y - 1][player.x] & TILE_WALL)) // allowed player move?
			{
				if (level[player.y - 1][player.x] & TILE_STONE) // next is stone?
				{
					if ((level[player.y - 2][player.x] & TILE_WALL) || (level[player.y - 2][player.x] & TILE_STONE)) // allowed stone move?
						break;																						 // skip move
					level[player.y - 1][player.x] ^= TILE_STONE;													 // stone remove
					level[player.y - 2][player.x] ^= TILE_STONE;													 // stone add
					if (lvl_cur == lvl_max)
						moves++; // score
				}
				player.y--; // move
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
				CheckDone(hWnd);
			}
			break;
		case 0x44: // L
		case 0x4C: // D
		case VK_RIGHT:
			if (player.x + 1 < TILE_COLS && !(level[player.y][player.x + 1] & TILE_WALL)) // allowed player move?
			{
				if (level[player.y][player.x + 1] & TILE_STONE) // next is stone?
				{
					if ((level[player.y][player.x + 2] & TILE_WALL) || (level[player.y][player.x + 2] & TILE_STONE)) // allowed stone move?
						break;																						 // skip move
					level[player.y][player.x + 1] ^= TILE_STONE;													 // stone remove
					level[player.y][player.x + 2] ^= TILE_STONE;													 // stone add
					if (lvl_cur == lvl_max)
						moves++; // score
				}
				player.x++; // move
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
				CheckDone(hWnd);
			}
			break;
		case 0x53: // S
		case 0x4B: // K
		case VK_DOWN:
			if (player.y + 1 < TILE_ROWS && !(level[player.y + 1][player.x] & TILE_WALL)) // allowed player move?
			{
				if (level[player.y + 1][player.x] & TILE_STONE) // next is stone?
				{
					if ((level[player.y + 2][player.x] & TILE_WALL) || (level[player.y + 2][player.x] & TILE_STONE)) // allowed stone move?
						break;																						 // skip move
					level[player.y + 1][player.x] ^= TILE_STONE;													 // stone remove
					level[player.y + 2][player.x] ^= TILE_STONE;													 // stone add
					if (lvl_cur == lvl_max)
						moves++; // score
				}
				player.y++; // move
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
				CheckDone(hWnd);
			}
			break;
		case VK_ESCAPE:
			ShowWindow(hWnd, SW_MINIMIZE);
			break;
		}
		break;
	case WM_CLOSE:
	{
		RegKey regkey(HKEY_CURRENT_USER, REG_KEY_NAME);
		if (!regkey.IsOpen()) // first time save?
		{
			if (MessageBox(hWnd, L"Save game ?", lpWindowTitle, MB_YESNO | MB_ICONQUESTION) == IDYES)
				regkey.OpenCreate(HKEY_CURRENT_USER, REG_KEY_NAME);
		}
		if (regkey.IsOpen())
		{
			regkey.SetDword(REG_VALUE_LVL_CUR, lvl_cur);
			regkey.SetDword(REG_VALUE_LVL_MAX, lvl_max);
			regkey.SetDword(REG_VALUE_SPRITES, sprites);
			regkey.SetDword(REG_VALUE_MOVES, moves);
			regkey.SetDword(REG_VALUE_CRC, moves ^ lvl_max ^ GetUserSalt());
		}
		break;
	}
	case WM_DESTROY:
		SelectObject(hSpritesMask, hBitmapMaskOld);
		SelectObject(hSprites, hBitmapOld);
		DeleteDC(hSpritesMask);
		DeleteDC(hSprites);
		DeleteObject((HGDIOBJ)hBitmapMask);
		DeleteObject((HGDIOBJ)hBitmap);
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	// only one instance allowed
	HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, lpClassName); // instance already exists?
	if (hMutex)													 // second instance
	{
		HWND hWnd = FindWindow(lpClassName, lpWindowTitle); // find window
		if (hWnd)
		{
			if (IsIconic(hWnd)) // window is minimized?
				ShowWindow(hWnd, SW_RESTORE);
			SetForegroundWindow(hWnd);
		}
		return 0; // exit this instance
	}
	else											   // first instance
		hMutex = CreateMutex(NULL, TRUE, lpClassName); // create mutex

	// create class
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_LOADTRANSPARENT);
	wcex.hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_LOADTRANSPARENT);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpfnWndProc = (WNDPROC)WindowProcedure;
	wcex.lpszClassName = lpClassName;
	if (!RegisterClassEx(&wcex))
		return -1;

	// create main window
	DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_COMPOSITED; // WS_EX_COMPOSITED - enables double-buffering - removes flickering
	HWND hWnd = CreateWindowEx(dwExStyle, lpClassName, lpWindowTitle, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, SCENE_WIDTH, SCENE_HEIGHT, HWND_DESKTOP, (HMENU)NULL, hInstance, NULL);
	if (!hWnd)
	{
		UnregisterClass(lpClassName, hInstance);
		return -2;
	}
	ShowWindow(hWnd, SW_SHOW); // show window
	UpdateWindow(hWnd);		   // repaint window

	// enter message loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// exit
	UnregisterClass(lpClassName, hInstance);
	ReleaseMutex(hMutex);
	return 0;
}
