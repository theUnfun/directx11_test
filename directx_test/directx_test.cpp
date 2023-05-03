#include <iostream>
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <cassert>
#include <d3dcompiler.h>
//#include <d3dx11.h>

const int Error = -1;

struct float3
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
};

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
	RECT rc;
	GetClientRect(hWnd, &rc);
	UINT width = rc.right - rc.left;   // �������� ������
	UINT height = rc.bottom - rc.top;  // � ������ ����
	UINT createDeviceFlags = 0;

	// ��� �� ������� ������ �������������� ������ DirectX
	D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};

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

	HRESULT hr =
	    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevels, 1,
	                                  D3D11_SDK_VERSION, &sd, pSwapChain, pd3dDevice, nullptr, pImmediateContext);
	if (FAILED(hr))
		return false;

	hr = (*pSwapChain)->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)pBackBuffer);
	if (FAILED(hr))
		return false;
	return true;
}

bool RenderTargetView(ID3D11Device* device, ID3D11Texture2D* buffer, ID3D11RenderTargetView** rtv)
{
	HRESULT hr = device->CreateRenderTargetView(buffer, nullptr, rtv);
	if (FAILED(hr))
		return false;
	return true;
}


void Render(ID3D11DeviceContext* pImmediateContext,
            ID3D11RenderTargetView* pRenderTargetView,
            IDXGISwapChain* pSwapChain,
            ID3D11InputLayout* pVertexLayout,
            ID3D11Buffer* pVertexBuffer,
            ID3D11VertexShader* pVertexShader,
            ID3D11PixelShader* pPixelShader)
{
	float ClearColor[4] = {0.0f, 0.0f, 1.0f, 1.0f};  // �������, �������, �����, �����-�����
	pImmediateContext->ClearRenderTargetView(pRenderTargetView, ClearColor);

	pImmediateContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);

	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)320;
	vp.Height = (FLOAT)240;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	pImmediateContext->RSSetViewports(1, &vp);


	UINT stride = sizeof(float3);
	UINT offset = 0;
	pImmediateContext->IASetInputLayout(pVertexLayout);
	pImmediateContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
	pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pImmediateContext->VSSetShader(pVertexShader, NULL, 0);
	pImmediateContext->PSSetShader(pPixelShader, NULL, 0);

	pImmediateContext->Draw(3, 0);

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

bool CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob;

	HRESULT hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, D3DCOMPILE_ENABLE_STRICTNESS,
	                                0, ppBlobOut, pErrorBlob.GetAddressOf());
	/*hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel, dwShaderFlags, 0, NULL, ppBlobOut,
	                           &pErrorBlob, NULL);*/
	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		return false;
	}

	return true;
}

bool CreateShadersAndInputLayout(ID3D11Device* pd3dDevice,
                                 ID3D11VertexShader** pVertexShader,
                                 ID3D11PixelShader** pPixelShader,
                                 ID3D11InputLayout** pVertexLayout)
{
	Microsoft::WRL::ComPtr<ID3DBlob> pVSBlob;
	if (!CompileShaderFromFile(L"shaders.fx", "VS", "vs_5_0", pVSBlob.GetAddressOf()))
	{
		return false;
	}
	HRESULT hr = pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, pVertexShader);
	if (FAILED(hr))
	{
		return false;
	}


	D3D11_INPUT_ELEMENT_DESC layout[] = {
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	    /* ������������� ���, ������������� ������, ������, �������� ���� (0-15), ����� ������ ������
	       � ������ ������, ����� ��������� ����� (�� �����), InstanceDataStepRate (�� �����) */
	};
	UINT numElements = ARRAYSIZE(layout);

	// �������� ������� ������
	hr = pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
	                                   pVertexLayout);
	if (FAILED(hr))
		return false;

	Microsoft::WRL::ComPtr<ID3DBlob> pPSBlob;
	if (!CompileShaderFromFile(L"shaders.fx", "PS", "ps_5_0", pPSBlob.GetAddressOf()))
	{
		return false;
	}
	hr = pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, pPixelShader);
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

bool CreateGeometry(ID3D11Device* pd3dDevice, ID3D11Buffer** pVertexBuffer)
{
	// �������� ������ ������ (��� ������� ������������)
	float3 vertices[] = {float3{0.0f, 0.5f, 0.5f}, float3{0.5f, -0.5f, 0.5f}, float3{-0.5f, -0.5f, 0.5f}};

	D3D11_BUFFER_DESC bd;         // ���������, ����������� ����������� �����
	ZeroMemory(&bd, sizeof(bd));  // ������� ��
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(float3) * ARRAYSIZE(vertices);  // ������ ������
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;              // ��� ������ - ����� ������
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;          // ���������, ���������� ������ ������
	ZeroMemory(&InitData, sizeof(InitData));  // ������� ��
	InitData.pSysMem = vertices;              // ��������� �� ���� 3 �������
	// ����� ������ g_pd3dDevice ������� ������ ������ ������
	HRESULT hr = pd3dDevice->CreateBuffer(&bd, &InitData, pVertexBuffer);

	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

#if 0

bool CreateShaders()
{
	HRESULT hr = S_OK;

	// ���������� ���������� ������� �� �����
	ID3DBlob* pVSBlob = NULL;  // ��������������� ������ - ������ ����� � ����������� ������
	hr = CompileShaderFromFile(L"Urok2.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(
		    NULL, L"���������� �������������� ���� FX. ����������, ��������� ������ ��������� �� �����, ���������� ���� FX.",
		    L"������", MB_OK);
		return hr;
	}

	// �������� ���������� �������
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// ����������� ������� ������
	// ������� ����� ����� ��������� ��������� - ���������� � ������������, �������, ����, ����������
	// ��������. ������ ������ ���������, ����� ������ ��������� �������� �������, ������� �� ����������
	// ������������. ���� ������� (SimpleVertex) �������� ������ ���������� � ����������� � ������������.
	// ����� �� �� ��������� ��������� ������, ������� ����� �������������� ��� ��������� ���������� �
	// ����� ��������.
	D3D11_INPUT_ELEMENT_DESC layout[] = {
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	    /* ������������� ���, ������������� ������, ������, �������� ���� (0-15), ����� ������ ������
	       � ������ ������, ����� ��������� ����� (�� �����), InstanceDataStepRate (�� �����) */
	};
	UINT numElements = ARRAYSIZE(layout);

	// �������� ������� ������
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
	                                     &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// ����������� ������� ������
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// ���������� ����������� ������� �� �����
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile(L"Urok2.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(
		    NULL, L"���������� �������������� ���� FX. ����������, ��������� ������ ��������� �� �����, ���������� ���� FX.",
		    L"������", MB_OK);
		return hr;
	}

	// �������� ����������� �������
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// �������� ������ ������ (��� ������� ������������)
	SimpleVertex vertices[] = {XMFLOAT3(0.0f, 0.5f, 0.5f), XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT3(-0.5f, -0.5f, 0.5f)};

	D3D11_BUFFER_DESC bd;         // ���������, ����������� ����������� �����
	ZeroMemory(&bd, sizeof(bd));  // ������� ��
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 3;  // ������ ������
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;  // ��� ������ - ����� ������
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;          // ���������, ���������� ������ ������
	ZeroMemory(&InitData, sizeof(InitData));  // ������� ��
	InitData.pSysMem = vertices;              // ��������� �� ���� 3 �������
	// ����� ������ g_pd3dDevice ������� ������ ������ ������
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// ��������� ������ ������
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// ��������� ������� ��������� ������ � ������ (� ������ ������ - TRIANGLE LIST,
	// �. �. ����� 1-3 - ������ �����������, 4-6 - ������ � �. �. ������ ������ - TRIANGLE STRIP.
	// � ���� ������ ����� 1-3 - ������ �����������, 2-4 - ������, 3-5 - ������ � �. �.
	// � ���� ������� ���� ������ ���� �����������, ������� ������ ��������� �� ����� ��������.
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return S_OK;
}

#endif

int main()
{
	WindowClass window;

	if (!window)
	{
		return Error;
	}


	HWND main_window_hwnd = CreateWindowInstance();
	if (!main_window_hwnd)
	{
		return Error;
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
		return Error;
	}

	if (!RenderTargetView(pd3dDevice.Get(), pBackBuffer.Get(), pRenderTargetView.GetAddressOf()))
	{
		return Error;
	}

	Microsoft::WRL::ComPtr<ID3D11VertexShader> pVertexShader = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pPixelShader = nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> pVertexLayout = nullptr;

	if (!CreateShadersAndInputLayout(pd3dDevice.Get(), pVertexShader.GetAddressOf(), pPixelShader.GetAddressOf(),
	                                 pVertexLayout.GetAddressOf()))
	{
		return Error;
	}

	Microsoft::WRL::ComPtr<ID3D11Buffer> pVertexBuffer = nullptr;
	if (!CreateGeometry(pd3dDevice.Get(), pVertexBuffer.GetAddressOf()))
	{
		return Error;
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
			Render(pImmediateContext.Get(), pRenderTargetView.Get(), pSwapChain.Get(), pVertexLayout.Get(),
			       pVertexBuffer.Get(), pVertexShader.Get(), pPixelShader.Get());
		}
	}
}