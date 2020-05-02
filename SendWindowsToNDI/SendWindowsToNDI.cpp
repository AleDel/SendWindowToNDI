// SendWindowsToNDI.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//

#include "pch.h"
#include <iostream>
#include <string>

#include <Processing.NDI.Lib.h>

#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64

//#define UNICODE
using namespace std;

vector<HWND> vec;

BOOL CALLBACK mycallback(HWND hwnd, LPARAM lParam) {
	const DWORD TITLE_SIZE = 1024;
	WCHAR windowTitle[TITLE_SIZE];

	GetWindowTextW(hwnd, windowTitle, TITLE_SIZE);

	int length = ::GetWindowTextLength(hwnd);
	std::wstring title(&windowTitle[0]);


	if (!IsWindowVisible(hwnd) || length == 0) {

		return TRUE;
	}

	// Retrieve the pointer passed into this callback, and re-'type' it.
	// The only way for a C API to pass arbitrary data is by means of a void*.
	std::vector<std::wstring>& titles = *reinterpret_cast<std::vector<std::wstring>*>(lParam);
	titles.push_back(title);
	vec.push_back(hwnd);
	cout << "............ " << "[" << (int)titles.size() - 1 << "] " << hwnd << "..." << string(title.begin(), title.end()) << endl;

	return TRUE;
}

int SendWindow(HWND hWnd, const char* ndi_name)
{
	HDC hdcWindow;
	HDC hdcMemDC = NULL;
	HBITMAP hbmScreen = NULL;
	BITMAP bmpScreen;

	// Retrieve the handle to a display device context for the client 
	// area of the window. 
	hdcWindow = GetDC(hWnd);

	// Create a compatible DC which is used in a BitBlt from the window DC
	hdcMemDC = CreateCompatibleDC(hdcWindow);
	if (!hdcMemDC)
	{
		MessageBox(hWnd, L"CreateCompatibleDC has failed", L"Failed", MB_OK);
	}

	// Get the client area for size calculation
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	// We are going to open a Win32 wrapper at rcClient resolution at 29.97fps
	const int xres = rcClient.right - rcClient.left;
	const int yres = rcClient.bottom - rcClient.top;
	const int framerate_n = 30000;
	const int framerate_d = 1001;

	//This is the best stretch mode
	//SetStretchBltMode(hdcWindow, HALFTONE);

	// Create a compatible bitmap from the Window DC
	hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
	if (!hbmScreen)
	{
		MessageBox(hWnd, L"CreateCompatibleBitmap Failed", L"Failed", MB_OK);
	}

	// Select the compatible bitmap into the compatible memory DC.
	SelectObject(hdcMemDC, hbmScreen);

	///////////////////
	// Create an NDI source that is called "My Video" and is clocked to the video.
	NDIlib_send_create_t NDI_send_create_desc;
	NDI_send_create_desc.p_ndi_name = ndi_name;
	NDIlib_send_instance_t m_pNDI_send = NDIlib_send_create(&NDI_send_create_desc);
	if (!m_pNDI_send) { return 0; };

	cout << "\n";

	for (int x = 0, dx = (rand() & 7), cnt = 0;; x += dx)
	{
		// Get the system time
		wchar_t temp[128];
		swprintf(temp, 128, L"%06d", cnt++);
		wcout << temp << "\r";

		// Bit block transfer into our compatible memory DC.
		if (!BitBlt(hdcMemDC,
			0, 0,
			rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
			hdcWindow,
			0, 0,
			SRCCOPY))
		{
			MessageBox(hWnd, L"BitBlt has failed", L"Failed", MB_OK);
		}

		// Get the BITMAP from the HBITMAP
		GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

		BITMAPINFOHEADER   bi;

		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = bmpScreen.bmWidth;
		bi.biHeight = -bmpScreen.bmHeight;
		bi.biPlanes = 1;
		bi.biBitCount = 32;
		bi.biCompression = BI_RGB;
		bi.biSizeImage = 0;
		bi.biXPelsPerMeter = 0;
		bi.biYPelsPerMeter = 0;
		bi.biClrUsed = 0;
		bi.biClrImportant = 0;

		DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

		// Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
		// call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
		// have greater overhead than HeapAlloc.
		HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
		char *lpbitmap = (char *)GlobalLock(hDIB);

		// Gets the "bits" from the bitmap and copies them into a buffer 
		// which is pointed to by lpbitmap.
		GetDIBits(hdcWindow, hbmScreen, 0, (UINT)bmpScreen.bmHeight, lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

		// Describe the frame. Note another undocumented feature is that if you pass the pointer as NULL then it does not send the frame, 
		// but it does still clock it correctly as if you did :) I bet you are glad you are reading this example.
		NDIlib_video_frame_v2_t frame;
		frame.xres = xres;
		frame.yres = yres;
		frame.FourCC = NDIlib_FourCC_type_BGRA;
		frame.frame_rate_N = framerate_n;
		frame.frame_rate_D = framerate_d;
		frame.p_data = (uint8_t*)lpbitmap;
		frame.line_stride_in_bytes = xres * 4;

		// We now just send the frame
		if (m_pNDI_send)
			NDIlib_send_send_video_v2(m_pNDI_send, &frame);/**/

		//Free memory heap
		GlobalFree(hDIB);
	}

	//Clean up
	DeleteObject(hbmScreen);
	DeleteObject(hdcMemDC);
	ReleaseDC(hWnd, hdcWindow);

	return 0;
}

int main()
{
	std::cout << "\n" << endl;
	std::cout << "/////////////////////\n" << endl;
	std::cout << "Windows list!\n" << endl;

	//info
	/*const DWORD info_SIZE = 1024;
	WCHAR infobuf[info_SIZE];

	GetComputerNameW(infobuf, (DWORD*)&info_SIZE);
	wcout << L"Equipo: " << infobuf << endl;

	GetUserNameW(infobuf, (DWORD*)&info_SIZE);
	wcout << L"Usuario: " << infobuf << endl;
	*/

	//Enumera las ventanas
	std::vector<std::wstring> titles;
	EnumWindows(mycallback, reinterpret_cast<LPARAM>(&titles));

	//input seleccion
	int x;
	cout << "\nSelect a window: (number between " << 0 << " and " << (int)titles.size() - 1 << "): ";
	cin >> x;

	//check si se introdujo un numero correcto
	while (cin.fail() || x < 0 || x >(int)titles.size() - 1) {

		cout << "repetir: " << endl;
		cin.clear();
		cin.ignore(256, '\n');
		cin >> x;
	}
	cout << "ok" << endl;

	// retransmite ventana por ndi
	cout << "Broadcasting -------> " << vec[x] << " ... " << string(titles[x].begin(), titles[x].end());

	// Not required, but "correct" (see the SDK documentation.
	if (!NDIlib_initialize())
	{
		// Cannot run NDI. Most likely because the CPU is not sufficient (see SDK documentation).
		// you can check this directly with a call to NDIlib_is_supported_CPU()
		printf("Cannot run NDI.");
		return 0;
	}

	string b = string(titles[x].begin(), titles[x].end());
	SendWindow(vec[x], "__ndi");

	// Not required, but nice
	NDIlib_destroy();

	return 0;
}

// Ejecutar programa: Ctrl + F5 o menú Depurar > Iniciar sin depurar
// Depurar programa: F5 o menú Depurar > Iniciar depuración

// Sugerencias para primeros pasos: 1. Use la ventana del Explorador de soluciones para agregar y administrar archivos
//   2. Use la ventana de Team Explorer para conectar con el control de código fuente
//   3. Use la ventana de salida para ver la salida de compilación y otros mensajes
//   4. Use la ventana Lista de errores para ver los errores
//   5. Vaya a Proyecto > Agregar nuevo elemento para crear nuevos archivos de código, o a Proyecto > Agregar elemento existente para agregar archivos de código existentes al proyecto
//   6. En el futuro, para volver a abrir este proyecto, vaya a Archivo > Abrir > Proyecto y seleccione el archivo .sln
