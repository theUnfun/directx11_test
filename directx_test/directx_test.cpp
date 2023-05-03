#include <iostream>
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <cassert>
//#include <d3dx11.h>

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// ���������� ������ ���, ����� ���������� �������� ���������
	PAINTSTRUCT ps;
	HDC hdc;
	switch (message)
	{
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool RegisterWindowClass()
{
	// ����������� ������
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandle(nullptr);
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"myDirectx";
	wcex.hIconSm = nullptr;

	return RegisterClassEx(&wcex) != 0;
}

void UnregisterWindowClass()
{
	UnregisterClass(L"myDirectx", nullptr);
}


HWND CreateWindowInstance()
{
	RECT rc = {0, 0, 640, 480};
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	HWND hWnd = CreateWindow(L"myDirectx", L"myDirectx", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
	                         rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	if (hWnd)
	{
		ShowWindow(hWnd, SW_SHOWDEFAULT);
	}
	return hWnd;
}

bool CreateDeviceAndSwapchainAndImmediateContext(HWND hWnd,
                                                 ID3D11Device** pd3dDevice,
                                                 ID3D11DeviceContext** pImmediateContext,
                                                 IDXGISwapChain** pSwapChain,
                                                 ID3D11Texture2D** pBackBuffer)
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(hWnd, &rc);
	UINT width = rc.right - rc.left;   // �������� ������
	UINT height = rc.bottom - rc.top;  // � ������ ����
	UINT createDeviceFlags = 0;

	// ������ �� ��������� ������� ������� ������� ����� �������, ��� ���, ������� �����
	// �������� ����� ��������, ������������ � ������. ������ ���� ������������ ������
	// ��� ����, ����� ��������, ����� ����� ��������� 3D ������������ ��� ���������.
	// ���� �� ���������� �� ����� ������� � �������� ������� ����������. ���� ���������
	// ������, ������� ��������� ���. �� ����������, �� ��������� ������ ��� ����������
	// ������������ ���������� ���������, � ������� ������� ���� ��� ���������� ������.
	// �����, � ������� �� ����.
	D3D_DRIVER_TYPE driverTypes[] = {
	    D3D_DRIVER_TYPE_HARDWARE,
	    D3D_DRIVER_TYPE_WARP,
	    D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	// ��� �� ������� ������ �������������� ������ DirectX
	D3D_FEATURE_LEVEL featureLevels[] = {
	    D3D_FEATURE_LEVEL_11_0,
	    D3D_FEATURE_LEVEL_10_1,
	    D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	// ������ �� �������� ���������� DirectX. ��� ������ �������� ���������,
	// ������� ��������� �������� ��������� ������ � ����������� ��� � ������ ����.
	DXGI_SWAP_CHAIN_DESC sd;        // ���������, ����������� ���� ����� (Swap Chain)
	ZeroMemory(&sd, sizeof(sd));    // ������� ��
	sd.BufferCount = 1;             // � ��� ���� ������ �����
	sd.BufferDesc.Width = width;    // ������ ������
	sd.BufferDesc.Height = height;  // ������ ������
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // ������ ������� � ������
	sd.BufferDesc.RefreshRate.Numerator = 75;           // ������� ���������� ������
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // ���������� ������ - ������ �����
	sd.OutputWindow = hWnd;                            // ����������� � ������ ����
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;  // �� ������������� �����

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		D3D_DRIVER_TYPE driverType = driverTypes[driverTypeIndex];
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
		hr = D3D11CreateDeviceAndSwapChain(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
		                                   D3D11_SDK_VERSION, &sd, pSwapChain, pd3dDevice, &featureLevel, pImmediateContext);
		if (SUCCEEDED(hr))  // ���� ���������� ������� �������, �� ������� �� �����
			break;
	}
	if (FAILED(hr))
		return false;

	// ������ ������� ������ �����. �������� ��������, � SDK
	// RenderTargetOutput - ��� �������� �����, � RenderTargetView - ������.
	// ID3D11Texture2D* pBackBuffer = nullptr;


	hr = (*pSwapChain)->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)pBackBuffer);
	if (FAILED(hr))
		return false;

	//// � ��� ��������, ��� ��������� g_pd3dDevice �����
	//// �������������� ��� �������� ��������� ��������
	// hr = pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	// pBackBuffer->Release();
	// if (FAILED(hr))
	//	return hr;

	//// ���������� ������ ������� ������ � ��������� ����������
	// g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

	//// ��������� ��������
	// D3D11_VIEWPORT vp;
	// vp.Width = (FLOAT)width;
	// vp.Height = (FLOAT)height;
	// vp.MinDepth = 0.0f;
	// vp.MaxDepth = 1.0f;
	// vp.TopLeftX = 0;
	// vp.TopLeftY = 0;
	//// ���������� ������� � ��������� ����������
	// g_pImmediateContext->RSSetViewports(1, &vp);

	// return S_OK;
	return true;
}

bool RenderTargetView(ID3D11Device* device, ID3D11Texture2D* buffer, ID3D11RenderTargetView** rtv)
{
	HRESULT hr = device->CreateRenderTargetView(buffer, nullptr, rtv);
	if (FAILED(hr))
		return false;
	return true;
}


void Render(ID3D11DeviceContext* pImmediateContext, ID3D11RenderTargetView* pRenderTargetView, IDXGISwapChain* pSwapChain)
{
	pImmediateContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);

	// ��������� ��������
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)640;
	vp.Height = (FLOAT)480;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	// ���������� ������� � ��������� ����������
	pImmediateContext->RSSetViewports(1, &vp);

	// ������ ������� ������ �����
	float ClearColor[4] = {0.0f, 0.0f, 1.0f, 1.0f};  // �������, �������, �����, �����-�����
	pImmediateContext->ClearRenderTargetView(pRenderTargetView, ClearColor);
	// ��������� ������ ����� �� �����
	pSwapChain->Present(0, 0);
}

class WindowClass
{
public:
	WindowClass()
	{
		assert(!is_init);
		is_init = RegisterWindowClass();
	}

	~WindowClass()
	{
		if (is_init)
		{
			UnregisterWindowClass();
		}
	}

	explicit operator bool() const
	{
		return is_init;
	}


private:
	bool is_init = false;
};

int main()
{
	WindowClass window;

	if (!window)
	{
		return -1;
	}


	HWND main_window_hwnd = CreateWindowInstance();
	if (!main_window_hwnd)
	{
		return -1;
	}

	Microsoft::WRL::ComPtr<ID3D11Device> pd3dDevice = nullptr;  // ���������� (��� �������� ��������)
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pImmediateContext = nullptr;  // �������� ���������� (���������)
	Microsoft::WRL::ComPtr<IDXGISwapChain> pSwapChain = nullptr;  // ���� ����� (������ � �������)
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pBackBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRenderTargetView = nullptr;  // ������ ������� ������

	if (!CreateDeviceAndSwapchainAndImmediateContext(main_window_hwnd, pd3dDevice.GetAddressOf(),
	                                                 pImmediateContext.GetAddressOf(), pSwapChain.GetAddressOf(),
	                                                 pBackBuffer.GetAddressOf()))
	{
		return -1;
	}

	if (!RenderTargetView(pd3dDevice.Get(), pBackBuffer.Get(), pRenderTargetView.GetAddressOf()))
	{
		return -1;
	}

	ShowWindow(main_window_hwnd, SW_SHOWDEFAULT);

	MSG msg = {0};
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render(pImmediateContext.Get(), pRenderTargetView.Get(), pSwapChain.Get());
		}
	}
}