#include <iostream>
#include <vector>
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <cassert>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DDSTextureLoader.h>

const int Error = -1;

using float3 = DirectX::XMFLOAT3;
using float4 = DirectX::XMFLOAT4;
using float2 = DirectX::XMFLOAT2;
using matrix = DirectX::XMMATRIX;
using uint2 = DirectX::XMUINT2;
using Microsoft::WRL::ComPtr;

struct VertexInput
{
	float3 position;
	float3 color;
	float3 normal;
	float2 uv;
};

struct Geometry
{
	ComPtr<ID3D11Buffer> vertex_buffer;
	ComPtr<ID3D11Buffer> index_buffer;
};

struct Light
{
	float4 direction;
	float4 color;
};

struct Material
{
	struct ShaderParameters
	{
		matrix world_matrix;
	};

	ComPtr<ID3D11VertexShader> vertex_shader;
	ComPtr<ID3D11PixelShader> pixel_shader;
	ComPtr<ID3D11InputLayout> vertex_layout;
	ComPtr<ID3D11Buffer> constant_buffer;
	ComPtr<ID3D11ShaderResourceView> texture_srv;
};

struct Object
{
	matrix transform;

	Geometry geometry;

	Material material;
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
		auto toXMVECTOR = [](const float3& value) { return DirectX::XMVectorSet(value.x, value.y, value.z, 0); };

		view_matrix = DirectX::XMMatrixLookAtLH(toXMVECTOR(eye), toXMVECTOR(at), toXMVECTOR(up));

		projection_matrix =
		    DirectX::XMMatrixPerspectiveFovLH(fov, resolution.x / (float)resolution.y, near_plane, far_plane);
	}

	matrix view_matrix;
	matrix projection_matrix;
};

struct BackBuffer
{
	ComPtr<IDXGISwapChain> swap_chain;
	ComPtr<ID3D11Texture2D> color;
	ComPtr<ID3D11RenderTargetView> rtv;

	ComPtr<ID3D11Texture2D> depth;
	ComPtr<ID3D11DepthStencilView> dsv;
};

struct Renderer
{
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> immediate_context;
};

bool CreateGeometry(Renderer& renderer, Geometry& geometry)
{
	VertexInput vertices[] = {
	    VertexInput{{-1.0f, 1.0f, -1.0f}, {0.1f, 0.8f, 0.7f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	    VertexInput{{1.0f, 1.0f, -1.0f}, {0.5f, 0.2f, 0.3f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	    VertexInput{{1.0f, 1.0f, 1.0f}, {0.2f, 0.8f, 0.8f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
	    VertexInput{{-1.0f, 1.0f, 1.0f}, {0.3f, 0.4f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
	    VertexInput{{-1.0f, -1.0f, -1.0f}, {0.3f, 0.8f, 0.4f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
	    VertexInput{{1.0f, -1.0f, -1.0f}, {0.7f, 0.5f, 0.7f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
	    VertexInput{{1.0f, -1.0f, 1.0f}, {0.1f, 0.2f, 0.1f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
	    VertexInput{{-1.0f, -1.0f, 1.0f}, {0.9f, 0.3f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
	    VertexInput{{-1.0f, -1.0f, 1.0f}, {0.6f, 0.8f, 0.7f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	    VertexInput{{-1.0f, -1.0f, -1.0f}, {0.2f, 0.1f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	    VertexInput{{-1.0f, 1.0f, -1.0f}, {0.1f, 0.3f, 0.8f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	    VertexInput{{-1.0f, 1.0f, 1.0f}, {0.5f, 0.6f, 0.1f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
	    VertexInput{{1.0f, -1.0f, 1.0f}, {0.1f, 0.8f, 0.7f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	    VertexInput{{1.0f, -1.0f, -1.0f}, {0.9f, 0.1f, 0.2f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	    VertexInput{{1.0f, 1.0f, -1.0f}, {0.4f, 0.5f, 0.1f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	    VertexInput{{1.0f, 1.0f, 1.0f}, {0.4f, 0.1f, 0.7f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
	    VertexInput{{-1.0f, -1.0f, -1.0f}, {0.5f, 0.2f, 0.1f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
	    VertexInput{{1.0f, -1.0f, -1.0f}, {0.8f, 0.8f, 0.8f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
	    VertexInput{{1.0f, 1.0f, -1.0f}, {0.3f, 0.4f, 0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
	    VertexInput{{-1.0f, 1.0f, -1.0f}, {0.1f, 0.2f, 0.3f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
	    VertexInput{{-1.0f, -1.0f, 1.0f}, {0.7f, 0.1f, 0.7f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
	    VertexInput{{1.0f, -1.0f, 1.0f}, {0.5f, 0.2f, 0.6f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
	    VertexInput{{1.0f, 1.0f, 1.0f}, {0.1f, 0.3f, 0.7f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	    VertexInput{{-1.0f, 1.0f, 1.0f}, {0.2f, 0.4f, 0.4f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	};

	D3D11_BUFFER_DESC bd; 
	ZeroMemory(&bd, sizeof(bd)); 
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VertexInput) * ARRAYSIZE(vertices);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;               
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;         
	ZeroMemory(&InitData, sizeof(InitData));  
	InitData.pSysMem = vertices;              

	HRESULT hr = renderer.device->CreateBuffer(&bd, &InitData, geometry.vertex_buffer.GetAddressOf());

	if (FAILED(hr))
	{
		return false;
	}

	uint16_t indices[] = {3,  1,  0,  2,
	                      1,  3,

	                      6,  4,  5,  7,
	                      4,  6,

	                      11, 9,  8,  10,
	                      9,  11,

	                      14, 12, 13, 15,
	                      12, 14,

	                      19, 17, 16, 18,
	                      17, 19,

	                      22, 20, 21, 23,
	                      20, 22

	};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(uint16_t) * ARRAYSIZE(indices);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices; 

	hr = renderer.device->CreateBuffer(&bd, &InitData, geometry.index_buffer.GetAddressOf());
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

template <class Param>
bool CreateShaderParameters(const Renderer& renderer, ID3D11Buffer** buffer)
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Param);               
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;  
	bd.CPUAccessFlags = 0;
	HRESULT hr = renderer.device->CreateBuffer(&bd, nullptr, buffer);
	if (FAILED(hr))
	{
		return false;
	}
	return true;
}

bool CompileShaderFromFile(const WCHAR* file_name, LPCSTR entry_point, LPCSTR shader_model, ID3DBlob** blob_out)
{
	ComPtr<ID3DBlob> error_blob;

	HRESULT hr = D3DCompileFromFile(file_name, nullptr, nullptr, entry_point, shader_model, D3DCOMPILE_ENABLE_STRICTNESS, 0,
	                                blob_out, error_blob.GetAddressOf());
	if (FAILED(hr))
	{
		if (error_blob != nullptr)
			OutputDebugStringA((char*)error_blob->GetBufferPointer());
		return false;
	}

	return true;
}

bool CreateShadersAndInputLayout(Renderer& renderer, Material& material)
{
	ComPtr<ID3DBlob> VSBlob;
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

	D3D11_INPUT_ELEMENT_DESC layout[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	                                     {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	                                     {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
	                                     {"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0}};
	UINT numElements = ARRAYSIZE(layout);

		hr = renderer.device->CreateInputLayout(layout, numElements, VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(),
	                                        material.vertex_layout.GetAddressOf());
	if (FAILED(hr))
		return false;

	ComPtr<ID3DBlob> PSblob;
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


struct Scene
{
	std::vector<Object> pyramids;
	Camera camera;
	Light light[2];
	ComPtr<ID3D11Buffer> constant_buffer;
	ComPtr<ID3D11Texture2D> texture;
	ComPtr<ID3D11ShaderResourceView> texture_srv;
	ComPtr<ID3D11SamplerState> sampler_state;

	struct ShaderParameters
	{
		matrix view_matrix;        
		matrix projection_matrix;  
		float4 light_direction[2];
		float4 light_color[2];
	};

	bool Init(HWND main_window, Renderer& renderer)
	{
		RECT rc;
		GetClientRect(main_window, &rc);
		UINT width = rc.right - rc.left;   
		UINT height = rc.bottom - rc.top;  

		camera.Set({5.0f, 4.0f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}, {width, height}, 0.01f, 100.f,
		           DirectX::XM_PIDIV4);

		if (!CreateShaderParameters<Scene::ShaderParameters>(renderer, constant_buffer.GetAddressOf()))
		{
			return false;
		}

		HRESULT hr =
		    DirectX::CreateDDSTextureFromFile(renderer.device.Get(), L"texture.dds", nullptr, texture_srv.GetAddressOf());
		if (FAILED(hr))
		{
			return false;
		}

				D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory(&sampDesc, sizeof(sampDesc));
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		
		hr = renderer.device->CreateSamplerState(&sampDesc, sampler_state.GetAddressOf());
		if (FAILED(hr))
			return false;

				pyramids.resize(6);
		for (auto& pyramid : pyramids)
		{
			pyramid.transform = DirectX::XMMatrixIdentity();
			if (!CreateGeometry(renderer, pyramid.geometry))
			{
				return false;
			}
			if (!CreateShadersAndInputLayout(renderer, pyramid.material))
			{
				return false;
			}

			if (!CreateShaderParameters<Material::ShaderParameters>(renderer,
			                                                        pyramid.material.constant_buffer.GetAddressOf()))
			{
				return false;
			}

			pyramid.material.texture_srv = texture_srv;
		}

				light[0].color = {1.0f, 1.0f, 1.0f, 0.f};
		light[1].color = {1.0f, 0.0f, 0.0f, 0.f};

		return true;
	}

	void Update()
	{
		uint32_t time_cur = GetTickCount();
		if (time_start == 0)
			time_start = time_cur;
		float rotation = (time_cur - time_start) / 1000.0f;


		for (uint32_t i = 0; i < pyramids.size(); ++i)
		{
						matrix Orbit = DirectX::XMMatrixRotationY(-rotation + i * (DirectX::XM_PI * 2) / 6);
			
			matrix Spin = DirectX::XMMatrixRotationY(rotation * 2);
		
			matrix Translate = DirectX::XMMatrixTranslation(-3.0f, 0.0f, 0.0f);
		
			matrix Scale = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f);

			pyramids[i].transform = Scale * Spin * Translate * Orbit;

	}

		light[0].direction = {-0.577f, 0.577f, -0.577f, 1.f};
		light[1].direction = {0.0f, 0.0f, -1.0f, 1.f};

		matrix rotate_matrix = DirectX::XMMatrixRotationY(-2.0f * rotation);
		DirectX::XMVECTOR light_dir = DirectX::XMLoadFloat4(&light[1].direction);
		light_dir = DirectX::XMVector3Transform(light_dir, rotate_matrix);
		DirectX::XMStoreFloat4(&light[1].direction, light_dir);

		rotate_matrix = DirectX::XMMatrixRotationY(0.5f * rotation);
		light_dir = DirectX::XMLoadFloat4(&light[0].direction);
		light_dir = DirectX::XMVector3Transform(light_dir, rotate_matrix);
		DirectX::XMStoreFloat4(&light[0].direction, light_dir);
	}

	void UploadShaderParams(Renderer& renderer)
	{
		Scene::ShaderParameters sp;
		sp.view_matrix = DirectX::XMMatrixTranspose(camera.view_matrix);
		sp.projection_matrix = DirectX::XMMatrixTranspose(camera.projection_matrix);
		sp.light_color[0] = light[0].color;
		sp.light_direction[0] = light[0].direction;
		sp.light_color[1] = light[1].color;
		sp.light_direction[1] = light[1].direction;
		renderer.immediate_context->UpdateSubresource(constant_buffer.Get(), 0, nullptr, &sp, 0, 0);

		for (const auto& pyramid : pyramids)
		{
			Material::ShaderParameters sp;
			sp.world_matrix = DirectX::XMMatrixTranspose(pyramid.transform);
			renderer.immediate_context->UpdateSubresource(pyramid.material.constant_buffer.Get(), 0, nullptr, &sp, 0, 0);
		}
	}


private:
	uint32_t time_start = 0;
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
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
	RECT rc = {0, 0, 1280, 960};
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
	UINT width = rc.right - rc.left;  
	UINT height = rc.bottom - rc.top;  
	UINT createDeviceFlags = 0;

		D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};

		DXGI_SWAP_CHAIN_DESC sd;       
	ZeroMemory(&sd, sizeof(sd));   
	sd.BufferCount = 1;             
	sd.BufferDesc.Width = width;   
	sd.BufferDesc.Height = height; 
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  
	sd.BufferDesc.RefreshRate.Numerator = 75;          
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; 
	sd.OutputWindow = hWnd;                           
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE; 

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

	D3D11_TEXTURE2D_DESC desc_depth; 
	ZeroMemory(&desc_depth, sizeof(desc_depth));
	desc_depth.Width = width;  
	desc_depth.Height = height;  
	desc_depth.MipLevels = 1;    
	desc_depth.ArraySize = 1;
	desc_depth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;  
	desc_depth.SampleDesc.Count = 1;
	desc_depth.SampleDesc.Quality = 0;
	desc_depth.Usage = D3D11_USAGE_DEFAULT;
	desc_depth.BindFlags = D3D11_BIND_DEPTH_STENCIL; 
	desc_depth.CPUAccessFlags = 0;
	desc_depth.MiscFlags = 0;
	hr = renderer.device->CreateTexture2D(&desc_depth, nullptr, back_buffer.depth.GetAddressOf());

	if (FAILED(hr))
		return hr;

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;  
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = desc_depth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;

	hr = renderer.device->CreateDepthStencilView(back_buffer.depth.Get(), &descDSV, back_buffer.dsv.GetAddressOf());

	if (FAILED(hr))
		return hr;

	return true;
}

void Render(Renderer& renderer, BackBuffer& back_buffer, Scene& scene)
{
	float ClearColor[4] = {0.0f, 0.0f, 1.0f, 1.0f};  
	renderer.immediate_context->ClearRenderTargetView(back_buffer.rtv.Get(), ClearColor);
	renderer.immediate_context->ClearDepthStencilView(back_buffer.dsv.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	ID3D11RenderTargetView* rtvs[] = {back_buffer.rtv.Get()};
	renderer.immediate_context->OMSetRenderTargets(1, rtvs, back_buffer.dsv.Get());

	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)1280;
	vp.Height = (FLOAT)960;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	renderer.immediate_context->RSSetViewports(1, &vp);

	for (auto& pyramid : scene.pyramids)
	{
		UINT stride = sizeof(VertexInput);
		UINT offset = 0;
		renderer.immediate_context->IASetInputLayout(pyramid.material.vertex_layout.Get());
		renderer.immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		renderer.immediate_context->VSSetShader(pyramid.material.vertex_shader.Get(), nullptr, 0);
		ID3D11Buffer* cbs[] = {scene.constant_buffer.Get(), pyramid.material.constant_buffer.Get()};
		renderer.immediate_context->VSSetConstantBuffers(0, 2, cbs);
		renderer.immediate_context->PSSetConstantBuffers(0, 2, cbs);
		renderer.immediate_context->PSSetShader(pyramid.material.pixel_shader.Get(), nullptr, 0);
		ID3D11ShaderResourceView* srvs[] = {pyramid.material.texture_srv.Get()};
		renderer.immediate_context->PSSetShaderResources(0, 1, srvs);
		ID3D11SamplerState* samplers[] = {scene.sampler_state.Get()};
		renderer.immediate_context->PSSetSamplers(0, 1, samplers);
		ID3D11Buffer* vbs[] = {pyramid.geometry.vertex_buffer.Get()};
		renderer.immediate_context->IASetVertexBuffers(0, 1, vbs, &stride, &offset);
		renderer.immediate_context->IASetIndexBuffer(pyramid.geometry.index_buffer.Get(), DXGI_FORMAT_R16_UINT, 0);
		renderer.immediate_context->DrawIndexed(36, 0, 0);
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

	if (!CreateDeviceAndSwapchainAndImmediateContext(main_window, renderer, back_buffer))
	{
		return Error;
	}

	Scene scene;

	if (!scene.Init(main_window, renderer))
	{
		return Error;
	}

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
			scene.UploadShaderParams(renderer);
			Render(renderer, back_buffer, scene);
		}
	}
}