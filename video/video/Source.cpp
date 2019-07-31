#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <Dshow.h>
#include <control.h>
#pragma comment(lib, "strmiids")
#define WM_GRAPHNOTIFY WM_APP + 1
#define V_PAUSE 100
#define V_OPEN 200
enum PlaybackState{ STATE_RUNNING , STATE_PAUSED , STATE_STOPPED };

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OnFileOpen(HWND hwnd);
int i = 0;
IGraphBuilder *pGraph = NULL;
IMediaControl *pControl = NULL;
IMediaEventEx   *pEvent = NULL;
IVideoWindow *pVidWin = NULL;
HRESULT hr;
PlaybackState state;
WCHAR szFileName[MAX_PATH];
IVMRWindowlessControl *pWc = NULL;

HRESULT InitWindowlessVMR( 
	HWND hwndApp,                  // Window to hold the video. 
	IGraphBuilder* pGraph,         // Pointer to the Filter Graph Manager. 
	IVMRWindowlessControl** ppWc   // Receives a pointer to the VMR.
) 
{ 
	if (!pGraph || !ppWc) 
	{
		return E_POINTER;
	}
	IBaseFilter* pVmr = NULL; 
	IVMRWindowlessControl* pWc = NULL; 
	// Create the VMR. 
	HRESULT hr = CoCreateInstance(CLSID_VideoMixingRenderer, NULL, 
		CLSCTX_INPROC, IID_IBaseFilter, (void**)&pVmr); 
	if (FAILED(hr))
	{
		return hr;
	}

	// Add the VMR to the filter graph.
	hr = pGraph->AddFilter(pVmr, L"Video Mixing Renderer"); 
	if (FAILED(hr)) 
	{
		pVmr->Release();
		return hr;
	}
	// Set the rendering mode.  
	IVMRFilterConfig* pConfig; 
	hr = pVmr->QueryInterface(IID_IVMRFilterConfig, (void**)&pConfig); 
	if (SUCCEEDED(hr)) 
	{ 
		hr = pConfig->SetRenderingMode(VMRMode_Windowless); 
		pConfig->Release(); 
	}
	if (SUCCEEDED(hr))
	{
		// Set the window. 
		hr = pVmr->QueryInterface(IID_IVMRWindowlessControl, (void**)&pWc);
		if( SUCCEEDED(hr)) 
		{ 
			hr = pWc->SetVideoClippingWindow(hwndApp); 
			if (SUCCEEDED(hr))
			{
				*ppWc = pWc; // Return this as an AddRef'd pointer. 
				pWc->Release();
			}
			else
			{
				// An error occurred, so release the interface.
			}
		} 
	} 
	pVmr->Release(); 
	return hr; 
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	// Register the window class.
	const wchar_t CLASS_NAME[]  = L"Sample Window Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc   = WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	RegisterClass(&wc);

	// Create the window.

	HWND hwnd = CreateWindowEx(
		0,                              // Optional window styles.
		CLASS_NAME,                     // Window class
		L"Learn to Program Windows",    // Window text
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,            // Window style

														  // Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		NULL,       // Parent window    
		NULL,       // Menu
		hInstance,  // Instance handle
		NULL        // Additional application data
	);

	if (hwnd == NULL)
	{
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);

	// Run the message loop.

	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

void release()
{
	pControl->Release();
	pEvent->Release();
	pGraph->Release();
}

HWND hd_pause,hd_open;
RECT rcSrc,rcDest;
long lWidth , lHeight;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch ( uMsg )
	{
		case WM_DESTROY:
			release();
			CoUninitialize();
			PostQuitMessage(0);
			return 0;

		case WM_CREATE:
		{
			state = STATE_STOPPED;
			hd_open = CreateWindow(L"button" , L"Video" , WS_CHILD | WS_VISIBLE , 50 , 50 , 80 , 20 , hwnd , (HMENU)V_OPEN , NULL , NULL);
			hd_pause = CreateWindow(L"button" , L"Pause" , WS_CHILD | WS_VISIBLE , 450 , 50 , 80 , 20 , hwnd , (HMENU)V_PAUSE , NULL , NULL);
			hr = CoInitializeEx(NULL , COINIT_MULTITHREADED);
			// Initialize the COM library.
			if ( FAILED(hr) )
			{
				MessageBox(hwnd , L"ERROR - Could not initialize COM library" , L"ERROR" , MB_OK);
				return 0;
			}
			// Create the filter graph manager and query for interfaces.
			hr = CoCreateInstance(CLSID_FilterGraph , NULL , CLSCTX_INPROC_SERVER ,
				IID_IGraphBuilder , (void **)&pGraph);
			if ( FAILED(hr) )
			{
				MessageBox(hwnd , L"ERROR - Could not create the Filter Graph Manager." , L"ERROR" , MB_OK);
				return 0;
			}

			hr = pGraph->QueryInterface(IID_IMediaControl , (void **)&pControl);
			hr = pGraph->QueryInterface(IID_IMediaEvent , (void **)&pEvent);
			pEvent->SetNotifyWindow((OAHWND)hwnd , WM_GRAPHNOTIFY , NULL);

			//if ( FAILED(hr) ) MessageBox(hwnd , L"Windowless Initialization failed" , L"Error" , MB_OK);
			return 0;
		}

		case WM_SIZE:
			GetClientRect(hwnd , &rcDest);
			if ( state == STATE_RUNNING || state == STATE_PAUSED )
			{
				pWc->SetVideoPosition(&rcSrc , &rcDest);
				//if(state==STATE_PAUSED ) pControl->
			}
			break;

		case WM_COMMAND:
		{
			switch ( LOWORD(wParam) )
			{
				case V_OPEN:
				{
					if ( state == STATE_RUNNING || state == STATE_PAUSED )
					{
						pControl->StopWhenReady();
						//release();
					}

					hr = InitWindowlessVMR(hwnd , pGraph , &pWc);
					if ( SUCCEEDED(hr) )
					{
						OnFileOpen(hwnd);
						// Build the graph. IMPORTANT: Change this string to a file on your system.
						hr = pGraph->RenderFile(szFileName , NULL);

						hr = pWc->GetNativeVideoSize(&lWidth , &lHeight , NULL , NULL);
						if ( SUCCEEDED(hr) )
						{
							rcSrc = { 0,0,lWidth,lHeight };
							// Set the video position.
							hr = pWc->SetVideoPosition(&rcSrc , &rcDest);

							if ( SUCCEEDED(hr) )
							{
								// Run the graph.
								hr = pControl->Run();
								if ( SUCCEEDED(hr) )
								{
									state = STATE_RUNNING;
								}
							}
							else MessageBox(hwnd , L"Unable to load media file" , L"Error" , MB_OK);
							//pWc->Release();
						}
						else MessageBox(hwnd , L"Unable to load media file" , L"Error" , MB_OK);

					}
				}break;

				case V_PAUSE:
					if ( state == STATE_RUNNING )
					{
						hr = pControl->Pause();
						state = STATE_PAUSED;
						SetWindowText(hd_pause , L"Play");
					}
					else if ( state == STATE_PAUSED )
					{
						hr = pControl->Run();
						state = STATE_RUNNING;
						SetWindowText(hd_pause , L"Pause");
					}
					break;
			}
		}break;

		case WM_GRAPHNOTIFY:
		{
		// Disregard if we don't have an IMediaEventEx pointer.
			if ( pEvent == NULL )
			{
				return 0;
			}
			// Get all the events
			long evCode = 0;
			LONG_PTR param1 = 0 , param2 = 0;
			HRESULT hr;

			while ( SUCCEEDED(pEvent->GetEvent(&evCode , &param1 , &param2 , 0)) )
			{
				switch ( evCode )
				{
					case EC_COMPLETE:
						state = STATE_STOPPED;
						MessageBox(hwnd , L"Playback complete" , L"Complete" , MB_OK);
						break;

					case EC_USERABORT:
						state = STATE_STOPPED;
						MessageBox(hwnd , L"User Aborted Playback" , L"Error" , MB_OK);
						break;

					case EC_ERRORABORT:
						state = STATE_STOPPED;
						MessageBox(hwnd , L"Error Aborted Playback" , L"Error" , MB_OK);
						PostQuitMessage(0);
						break;
				}
				pEvent->FreeEventParams(evCode , param1 , param2);
			}
		}break;

	/*	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

			EndPaint(hwnd, &ps);
		}*/
		return 0;

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void OnFileOpen(HWND hwnd)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	WCHAR szFileName[MAX_PATH];
	szFileName[0] = L'\0';

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpstrFilter = L"All (*.*)/0*.*/0";
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
		InvalidateRect(hwnd, NULL, FALSE);
		lstrcpyW(::szFileName , szFileName);
	}
}