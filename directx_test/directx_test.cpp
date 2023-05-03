#include <iostream>
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <cassert>
//#include <d3dx11.h>

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// Вызывается каждый раз, когда приложение получает сообщение
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
	// Регистрация класса
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
	UINT width = rc.right - rc.left;   // получаем ширину
	UINT height = rc.bottom - rc.top;  // и высоту окна
	UINT createDeviceFlags = 0;

	// Видите ли Микрософт создает учебные примеры таким образом, что код, который можно
	// написать одной строчкой, превращается в десять. Массив ниже используется просто
	// для того, чтобы выяснить, какой режим обработки 3D поддерживает анш компьютер.
	// Ниже мы проходимся по этому массиву и пытаемся создать устройство. Если произошла
	// ошибка, пробуем следующий тип. Всё примитивно, но поскольку сейчас все компьютеры
	// поддерживают хардварную обработку, в учебном примере этот код совершенно лишний.
	// Удачи, и спасибо за рыбу.
	D3D_DRIVER_TYPE driverTypes[] = {
	    D3D_DRIVER_TYPE_HARDWARE,
	    D3D_DRIVER_TYPE_WARP,
	    D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	// Тут мы создаем список поддерживаемых версий DirectX
	D3D_FEATURE_LEVEL featureLevels[] = {
	    D3D_FEATURE_LEVEL_11_0,
	    D3D_FEATURE_LEVEL_10_1,
	    D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	// Сейчас мы создадим устройства DirectX. Для начала заполним структуру,
	// которая описывает свойства переднего буфера и привязывает его к нашему окну.
	DXGI_SWAP_CHAIN_DESC sd;        // Структура, описывающая цепь связи (Swap Chain)
	ZeroMemory(&sd, sizeof(sd));    // очищаем ее
	sd.BufferCount = 1;             // у нас один задний буфер
	sd.BufferDesc.Width = width;    // ширина буфера
	sd.BufferDesc.Height = height;  // высота буфера
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // формат пикселя в буфере
	sd.BufferDesc.RefreshRate.Numerator = 75;           // частота обновления экрана
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // назначение буфера - задний буфер
	sd.OutputWindow = hWnd;                            // привязываем к нашему окну
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;  // не полноэкранный режим

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		D3D_DRIVER_TYPE driverType = driverTypes[driverTypeIndex];
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
		hr = D3D11CreateDeviceAndSwapChain(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
		                                   D3D11_SDK_VERSION, &sd, pSwapChain, pd3dDevice, &featureLevel, pImmediateContext);
		if (SUCCEEDED(hr))  // Если устройства созданы успешно, то выходим из цикла
			break;
	}
	if (FAILED(hr))
		return false;

	// Теперь создаем задний буфер. Обратите внимание, в SDK
	// RenderTargetOutput - это передний буфер, а RenderTargetView - задний.
	// ID3D11Texture2D* pBackBuffer = nullptr;


	hr = (*pSwapChain)->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)pBackBuffer);
	if (FAILED(hr))
		return false;

	//// Я уже упоминал, что интерфейс g_pd3dDevice будет
	//// использоваться для создания остальных объектов
	// hr = pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	// pBackBuffer->Release();
	// if (FAILED(hr))
	//	return hr;

	//// Подключаем объект заднего буфера к контексту устройства
	// g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

	//// Настройка вьюпорта
	// D3D11_VIEWPORT vp;
	// vp.Width = (FLOAT)width;
	// vp.Height = (FLOAT)height;
	// vp.MinDepth = 0.0f;
	// vp.MaxDepth = 1.0f;
	// vp.TopLeftX = 0;
	// vp.TopLeftY = 0;
	//// Подключаем вьюпорт к контексту устройства
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

	// Настройка вьюпорта
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)640;
	vp.Height = (FLOAT)480;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	// Подключаем вьюпорт к контексту устройства
	pImmediateContext->RSSetViewports(1, &vp);

	// Просто очищаем задний буфер
	float ClearColor[4] = {0.0f, 0.0f, 1.0f, 1.0f};  // красный, зеленый, синий, альфа-канал
	pImmediateContext->ClearRenderTargetView(pRenderTargetView, ClearColor);
	// Выбросить задний буфер на экран
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

	Microsoft::WRL::ComPtr<ID3D11Device> pd3dDevice = nullptr;  // Устройство (для создания объектов)
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pImmediateContext = nullptr;  // Контекст устройства (рисование)
	Microsoft::WRL::ComPtr<IDXGISwapChain> pSwapChain = nullptr;  // Цепь связи (буфера с экраном)
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pBackBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRenderTargetView = nullptr;  // Объект заднего буфера

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