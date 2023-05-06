#include <iostream>
#include <vector>
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <cassert>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
//#include <d3dx11.h>

const int Error = -1;

using float3 = DirectX::XMFLOAT3;
using matrix = DirectX::XMMATRIX;
using uint2 = DirectX::XMUINT2;

struct VertexInput
{
	float3 position;
	float3 color;
};

struct Geometry
{
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer;
};

struct Object
{
	matrix transform;  // Матрица мира

	Geometry geometry;
};

struct Camera
{
	void Set(const float3& eye,
	         const float3& at,
	         const float3& up,
	         const uint2& resolution,
	         float near_plane,
	         float far_plane,
	         float fov)
	{
		// Инициализация матрицы вида
		auto toXMVECTOR = [](const float3& value) { return DirectX::XMVectorSet(value.x, value.y, value.z, 0); };
		view_matrix = DirectX::XMMatrixLookAtLH(toXMVECTOR(eye), toXMVECTOR(at), toXMVECTOR(up));

		// Инициализация матрицы проекции
		projection_matrix =
		    DirectX::XMMatrixPerspectiveFovLH(fov, resolution.x / (float)resolution.y, near_plane, far_plane);
	}

	matrix view_matrix;        // Матрица вида
	matrix projection_matrix;  // Матрица проекции
};

struct ShaderParameters
{
	matrix world_matrix;       // Матрица мира
	matrix view_matrix;        // Матрица вида
	matrix projection_matrix;  // Матрица проекции
};

struct BackBuffer
{
	Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> color;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> depth;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv;
};

struct Material
{
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> vertex_layout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;
};


struct Renderer
{
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context;
};

bool CreateGeometry(Renderer& renderer, Geometry& geometry)
{
	VertexInput vertices[] = {
	    VertexInput{{0.0f, 1.5f, 0.0f}, {1.0f, 1.0f, 0.0f}},  VertexInput{{-1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
	    VertexInput{{1.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}}, VertexInput{{-1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
	    VertexInput{{1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
	};

	D3D11_BUFFER_DESC bd;         // Структура, описывающая создаваемый буфер
	ZeroMemory(&bd, sizeof(bd));  // очищаем ее
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VertexInput) * ARRAYSIZE(vertices);  // размер буфера
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;                   // тип буфера - буфер вершин
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;          // Структура, содержащая данные буфера
	ZeroMemory(&InitData, sizeof(InitData));  // очищаем ее
	InitData.pSysMem = vertices;              // указатель на наши 3 вершины
	// Вызов метода g_pd3dDevice создаст объект буфера вершин
	HRESULT hr = renderer.device->CreateBuffer(&bd, &InitData, geometry.vertex_buffer.GetAddressOf());

	if (FAILED(hr))
	{
		return false;
	}

	uint16_t indices[] = {
	    0, 2, 1, 0, 3, 4, 0, 1, 3, 0, 4, 2, 1, 2, 3, 2, 4, 3,
	};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(uint16_t) * ARRAYSIZE(indices);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;  // указатель на наш массив индексов

	hr = renderer.device->CreateBuffer(&bd, &InitData, geometry.index_buffer.GetAddressOf());
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

struct Scene
{
	std::vector<Object> pyramids;
	Camera camera;

	void Init(HWND main_window, Renderer& renderer)
	{
		RECT rc;
		GetClientRect(main_window, &rc);
		UINT width = rc.right - rc.left;   // получаем ширину
		UINT height = rc.bottom - rc.top;  // и высоту окна

		camera.Set({0.0f, 2.0f, -8.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {width, height}, 0.01f, 100.f,
		           DirectX::XM_PIDIV4);

		// Инициализация матрицы мира
		pyramids.resize(6);
		for (auto& pyramid : pyramids)
		{
			pyramid.transform = DirectX::XMMatrixIdentity();
			CreateGeometry(renderer, pyramid.geometry);
		}
	}

	void Update()
	{
		uint32_t time_cur = GetTickCount();
		if (time_start == 0)
			time_start = time_cur;
		float rotation = (time_cur - time_start) / 1000.0f;


		for (uint32_t i = 0; i < pyramids.size(); ++i)
		{
			// Матрица-орбита: позиция объекта
			matrix Orbit = DirectX::XMMatrixRotationY(-rotation + i * (DirectX::XM_PI * 2) / 6);
			// Матрица-спин: вращение объекта вокруг своей оси
			matrix Spin = DirectX::XMMatrixRotationY(rotation * 2);
			// Матрица-позиция: перемещение на три единицы влево от начала координат
			matrix Translate = DirectX::XMMatrixTranslation(-3.0f, 0.0f, 0.0f);
			// Матрица-масштаб: сжатие объекта в 2 раза
			matrix Scale = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f);

			// Результирующая матрица
			//  --Сначала мы в центре, в масштабе 1:1:1, повернуты по всем осям на 0.0f.
			//  --Сжимаем -> поворачиваем вокруг Y (пока мы еще в центре) -> переносим влево ->
			//  --снова поворачиваем вокруг Y.
			pyramids[i].transform = Scale * Spin * Translate * Orbit;

			// pyramids[i]
		}
	}


private:
	uint32_t time_start = 0;
};

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

bool CreateDeviceAndSwapchainAndImmediateContext(HWND hWnd, Renderer& renderer, BackBuffer& back_buffer)
{
	RECT rc;
	GetClientRect(hWnd, &rc);
	UINT width = rc.right - rc.left;   // получаем ширину
	UINT height = rc.bottom - rc.top;  // и высоту окна
	UINT createDeviceFlags = 0;

	// Тут мы создаем список поддерживаемых версий DirectX
	D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};

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

	HRESULT hr =
	    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevels, 1,
	                                  D3D11_SDK_VERSION, &sd, back_buffer.swap_chain.GetAddressOf(),
	                                  renderer.device.GetAddressOf(), nullptr, renderer.immediate_context.GetAddressOf());
	if (FAILED(hr))
		return false;

	hr = back_buffer.swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)back_buffer.color.GetAddressOf());
	if (FAILED(hr))
		return false;

	hr = renderer.device->CreateRenderTargetView(back_buffer.color.Get(), nullptr, back_buffer.rtv.GetAddressOf());
	if (FAILED(hr))
		return false;

	D3D11_TEXTURE2D_DESC desc_depth;  // Структура с параметрами
	ZeroMemory(&desc_depth, sizeof(desc_depth));
	desc_depth.Width = width;    // ширина и
	desc_depth.Height = height;  // высота текстуры
	desc_depth.MipLevels = 1;    // уровень интерполяции
	desc_depth.ArraySize = 1;
	desc_depth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;  // формат (размер пикселя)
	desc_depth.SampleDesc.Count = 1;
	desc_depth.SampleDesc.Quality = 0;
	desc_depth.Usage = D3D11_USAGE_DEFAULT;
	desc_depth.BindFlags = D3D11_BIND_DEPTH_STENCIL;  // вид - буфер глубин
	desc_depth.CPUAccessFlags = 0;
	desc_depth.MiscFlags = 0;
	hr = renderer.device->CreateTexture2D(&desc_depth, nullptr, back_buffer.depth.ReleaseAndGetAddressOf());

	if (FAILED(hr))
		return hr;

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;  // Структура с параметрами
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = desc_depth.Format;  // формат как в текстуре
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	// При помощи заполненной структуры-описания и текстуры создаем объект буфера глубин
	hr = renderer.device->CreateDepthStencilView(back_buffer.depth.Get(), &descDSV, back_buffer.dsv.GetAddressOf());

	if (FAILED(hr))
		return hr;

	return true;
}

void UpdateShaderParameters(Renderer& renderer, const Material& material, const Camera& camera, const Object& object)
{
	ShaderParameters sp;
	sp.world_matrix = DirectX::XMMatrixTranspose(object.transform);
	sp.view_matrix = DirectX::XMMatrixTranspose(camera.view_matrix);
	sp.projection_matrix = DirectX::XMMatrixTranspose(camera.projection_matrix);

	renderer.immediate_context->UpdateSubresource(material.constant_buffer.Get(), 0, nullptr, &sp, 0, 0);
}

void Render(Renderer& renderer, BackBuffer& back_buffer, Material& material, Scene& scene)
{
	float ClearColor[4] = {0.0f, 0.0f, 1.0f, 1.0f};  // красный, зеленый, синий, альфа-канал
	renderer.immediate_context->ClearRenderTargetView(back_buffer.rtv.Get(), ClearColor);
	renderer.immediate_context->ClearDepthStencilView(back_buffer.dsv.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	ID3D11RenderTargetView* rtvs[] = {back_buffer.rtv.Get()};
	renderer.immediate_context->OMSetRenderTargets(1, rtvs, back_buffer.dsv.Get());

	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)640;
	vp.Height = (FLOAT)480;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	renderer.immediate_context->RSSetViewports(1, &vp);

	UINT stride = sizeof(VertexInput);
	UINT offset = 0;
	renderer.immediate_context->IASetInputLayout(material.vertex_layout.Get());
	renderer.immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer.immediate_context->VSSetShader(material.vertex_shader.Get(), nullptr, 0);
	ID3D11Buffer* cbs[] = {material.constant_buffer.Get()};
	renderer.immediate_context->VSSetConstantBuffers(0, 1, cbs);
	renderer.immediate_context->PSSetShader(material.pixel_shader.Get(), nullptr, 0);

	// Выбросить задний буфер на экран
	for (auto& pyramid : scene.pyramids)
	{
		UpdateShaderParameters(renderer, material, scene.camera ,pyramid);
		ID3D11Buffer* vbs[] = {pyramid.geometry.vertex_buffer.Get()};
		renderer.immediate_context->IASetVertexBuffers(0, 1, vbs, &stride, &offset);
		renderer.immediate_context->IASetIndexBuffer(pyramid.geometry.index_buffer.Get(), DXGI_FORMAT_R16_UINT, 0);
		renderer.immediate_context->DrawIndexed(18, 0, 0);
	}

	back_buffer.swap_chain->Present(0, 0);
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
	if (FAILED(hr))
	{
		if (pErrorBlob != nullptr)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		return false;
	}

	return true;
}

bool CreateShadersAndInputLayout(Renderer& renderer, Material& material)
{
	Microsoft::WRL::ComPtr<ID3DBlob> VSBlob;
	if (!CompileShaderFromFile(L"shaders.fx", "VS", "vs_5_0", VSBlob.GetAddressOf()))
	{
		return false;
	}
	HRESULT hr = renderer.device->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), nullptr,
	                                                 material.vertex_shader.GetAddressOf());
	if (FAILED(hr))
	{
		return false;
	}


	D3D11_INPUT_ELEMENT_DESC layout[] = {
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	    {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	    /* семантическое имя, семантический индекс, размер, входящий слот (0-15), адрес начала данных
	       в буфере вершин, класс входящего слота (не важно), InstanceDataStepRate (не важно) */
	};
	UINT numElements = ARRAYSIZE(layout);

	// Создание шаблона вершин
	hr = renderer.device->CreateInputLayout(layout, numElements, VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(),
	                                        material.vertex_layout.GetAddressOf());
	if (FAILED(hr))
		return false;

	Microsoft::WRL::ComPtr<ID3DBlob> PSblob;
	if (!CompileShaderFromFile(L"shaders.fx", "PS", "ps_5_0", PSblob.GetAddressOf()))
	{
		return false;
	}
	hr = renderer.device->CreatePixelShader(PSblob->GetBufferPointer(), PSblob->GetBufferSize(), nullptr,
	                                        material.pixel_shader.GetAddressOf());
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

bool CreateShaderParameters(const Renderer& renderer, Material& material)
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ShaderParameters);    // размер буфера = размеру структуры
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;  // тип - константный буфер
	bd.CPUAccessFlags = 0;
	HRESULT hr = renderer.device->CreateBuffer(&bd, nullptr, material.constant_buffer.GetAddressOf());
	if (FAILED(hr))
	{
		return false;
	}
	return true;
}

int main()
{
	WindowClass window;

	if (!window)
	{
		return Error;
	}


	HWND main_window = CreateWindowInstance();
	if (!main_window)
	{
		return Error;
	}

	Renderer renderer;
	BackBuffer back_buffer;
	Material material;


	if (!CreateDeviceAndSwapchainAndImmediateContext(main_window, renderer, back_buffer))
	{
		return Error;
	}

	if (!CreateShadersAndInputLayout(renderer, material))
	{
		return Error;
	}

	if (!CreateShaderParameters(renderer, material))
	{
		return Error;
	}

	Scene scene;

	scene.Init(main_window, renderer);

	ShowWindow(main_window, SW_SHOWDEFAULT);

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
			scene.Update();
			Render(renderer, back_buffer, material, scene);
		}
	}
}