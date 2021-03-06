
#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "resource.h"

#include "terrainMesh.h"
#include <stdint.h>
#include <vector>
#include <map>

//#define DEBUG_VS   // Uncomment this line to debug D3D9 vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug D3D9 pixel shaders 

#pragma comment(lib, "legacy_stdio_definitions.lib")

#include "TexGenerator.h"

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CFirstPersonCamera          g_Camera;               // A model viewing camera
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = NULL;
CDXUTDialog                 g_HUD;                  // dialog for standard controls
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls

													// Direct3D 9 resources
ID3DXFont*                  g_pFont9 = NULL;
ID3DXSprite*                g_pSprite9 = NULL;
ID3DXEffect*                g_pEffect9 = NULL;
D3DXHANDLE                  g_hmWorldViewProjection;
D3DXHANDLE                  g_hmWorld;
D3DXHANDLE                  g_hfTime;


TerrainMesh* terrainMesh = NULL;


IDirect3DTexture9* terrainTex = NULL;



IDirect3DTexture9* newRT0 = NULL;
IDirect3DTexture9* newRT1 = NULL;
IDirect3DTexture9* newRT2 = NULL;
IDirect3DTexture9* sysRT = NULL;


VTGenerator* vtgen = NULL;


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           2
#define IDC_CHANGEDEVICE        3

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
	void* pUserContext);
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext);
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext);
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext);
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext);

bool CALLBACK IsD3D9DeviceAcceptable(D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
	bool bWindowed, void* pUserContext);
HRESULT CALLBACK OnD3D9CreateDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext);
HRESULT CALLBACK OnD3D9ResetDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext);
void CALLBACK OnD3D9FrameRender(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext);
void CALLBACK OnD3D9LostDevice(void* pUserContext);
void CALLBACK OnD3D9DestroyDevice(void* pUserContext);

void InitApp();
void RenderText();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// DXUT will create and use the best device (either D3D9 or D3D10) 
	// that is available on the system depending on which D3D callbacks are set below

	// Set DXUT callbacks
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackKeyboard(OnKeyboard);
	DXUTSetCallbackFrameMove(OnFrameMove);
	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);

	DXUTSetCallbackD3D9DeviceAcceptable(IsD3D9DeviceAcceptable);
	DXUTSetCallbackD3D9DeviceCreated(OnD3D9CreateDevice);
	DXUTSetCallbackD3D9DeviceReset(OnD3D9ResetDevice);
	DXUTSetCallbackD3D9DeviceLost(OnD3D9LostDevice);
	DXUTSetCallbackD3D9DeviceDestroyed(OnD3D9DestroyDevice);
	DXUTSetCallbackD3D9FrameRender(OnD3D9FrameRender);

	InitApp();
	DXUTInit(true, true, NULL); // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings(true, true);
	DXUTCreateWindow(L"TerrainEdit");
	DXUTCreateDevice(true, 1920, 1080);
	//DXUTCreateDevice(true, 1024, 1024);
	DXUTMainLoop(); // Enter into the DXUT render loop

	return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
	g_SettingsDlg.Init(&g_DialogResourceManager);
	g_HUD.Init(&g_DialogResourceManager);
	g_SampleUI.Init(&g_DialogResourceManager);

	g_HUD.SetCallback(OnGUIEvent); int iY = 10;
	g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22);
	g_HUD.AddButton(IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3);
	g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2);

	g_SampleUI.SetCallback(OnGUIEvent); iY = 10;
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos(5, 5);
	g_pTxtHelper->SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	g_pTxtHelper->DrawTextLine(DXUTGetFrameStats(DXUTIsVsyncEnabled()));
	g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());
	g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Rejects any D3D9 devices that aren't acceptable to the app by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D9DeviceAcceptable(D3DCAPS9* pCaps, D3DFORMAT AdapterFormat,
	D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
	// Skip backbuffer formats that don't support alpha blending
	IDirect3D9* pD3D = DXUTGetD3D9Object();
	if (FAILED(pD3D->CheckDeviceFormat(pCaps->AdapterOrdinal, pCaps->DeviceType,
		AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
		D3DRTYPE_TEXTURE, BackBufferFormat)))
		return false;

	// No fallback defined by this app, so reject any device that 
	// doesn't support at least ps2.0
	if (pCaps->PixelShaderVersion < D3DPS_VERSION(2, 0))
		return false;

	return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
	if (pDeviceSettings->ver == DXUT_D3D9_DEVICE)
	{
		IDirect3D9* pD3D = DXUTGetD3D9Object();
		D3DCAPS9 Caps;
		pD3D->GetDeviceCaps(pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &Caps);

		// If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
		// then switch to SWVP.
		if ((Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
			Caps.VertexShaderVersion < D3DVS_VERSION(1, 1))
		{
			pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		}


	}

	// For the first device created if its a REF device, optionally display a warning dialog box
	static bool s_bFirstTime = true;
	if (s_bFirstTime)
	{
		s_bFirstTime = false;
		if ((DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF) ||
			(DXUT_D3D10_DEVICE == pDeviceSettings->ver &&
				pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE))
			DXUTDisplaySwitchingToREFWarning(pDeviceSettings->ver);
	}

	return true;
}



uint32_t * indirectTexData[11];
IDirect3DTexture9* pFeedBackTex;
IDirect3DTexture9* pIndirectMap;
IDirect3DTexture9* pTestTex;

IDirect3DTexture9* indirectMap;

IDirect3DSurface9* pIndirectRT[11];



HRESULT CALLBACK OnD3D9CreateDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	HRESULT hr;

	V_RETURN(g_DialogResourceManager.OnD3D9CreateDevice(pd3dDevice));
	V_RETURN(g_SettingsDlg.OnD3D9CreateDevice(pd3dDevice));

	V_RETURN(D3DXCreateFont(pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		L"Arial", &g_pFont9));

	// Read the D3DX effect file
	WCHAR str[MAX_PATH];
	DWORD dwShaderFlags = D3DXFX_NOT_CLONEABLE;

#ifdef D3DXFX_LARGEADDRESS_HANDLE
	dwShaderFlags |= D3DXFX_LARGEADDRESSAWARE;
#endif

	ID3DXMesh* pmesh;
	D3DXCreateTeapot(pd3dDevice, &pmesh, NULL);
	
	struct MeshVertex
	{
		D3DXVECTOR3 pos;
		D3DXVECTOR3 nor;
	};

	UINT vertexcount = 0;
	vertexcount = pmesh->GetNumVertices();

	UINT indexcount = 0;
	indexcount = pmesh->GetNumFaces() * 3 ;

	MeshVertex* pVertex = new MeshVertex[vertexcount];

	void* pverts;
	pmesh->LockVertexBuffer(0, & pverts);
	memcpy(pVertex, pverts, sizeof(MeshVertex)* vertexcount);
	pmesh->UnlockVertexBuffer();


	WORD* pindex = new WORD[indexcount];
	void* pindices;
	pmesh->LockIndexBuffer(0, &pindices);
	memcpy(pindex, pindices, sizeof(WORD)* indexcount);
	pmesh->UnlockIndexBuffer();

	FILE* fmesh;
	fopen_s(&fmesh, "teapot", "wb");
	fwrite(&vertexcount, sizeof(UINT), 1, fmesh);
	fwrite(pVertex, sizeof(MeshVertex), vertexcount, fmesh);

	fwrite(&indexcount, sizeof(UINT), 1, fmesh);
	fwrite(pindex, sizeof(WORD), indexcount, fmesh);

	fclose(fmesh);

	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"vt.fx"));

	ID3DXBuffer* pError;
	D3DXCreateEffectFromFile(pd3dDevice, str, NULL, NULL, dwShaderFlags,
		NULL, &g_pEffect9, &pError);

	if (pError != nullptr)
	{
		char* error = (char*)pError->GetBufferPointer();
		OutputDebugStringA(error);
	}

	g_hmWorldViewProjection = g_pEffect9->GetParameterByName(NULL, "g_mWorldViewProjection");
	g_hmWorld = g_pEffect9->GetParameterByName(NULL, "g_mWorld");
	g_hfTime = g_pEffect9->GetParameterByName(NULL, "g_fTime");

	// Setup the camera's view parameters
	D3DXVECTOR3 vecEye(68.2821579+5.0f, 35.6292763, -61.6675453 + 5.0f);

	D3DXVECTOR3 vecAt(68.2470551, 35.6266937, -60.6681633 + 5.0f);

	g_Camera.SetViewParams(&vecEye, &vecAt);
	g_Camera.SetScalers(0.001f, 50.0f);

	D3DXCreateTextureFromFileA(pd3dDevice, "tex/Heightmap.png", &terrainTex);

	terrainMesh = new TerrainMesh();
	terrainMesh->Init(pd3dDevice);

	vtgen = new VTGenerator(pd3dDevice);

	pd3dDevice->CreateTexture(1024, 1024, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &pTestTex, NULL);

	uint32_t colorlevels[11] =
	{
		0xffffffff,
		0xffff0000,
		0xff00ff00,
		0xff0000ff,
		0x77777777,
		0x77770000,
		0x77007700,
		0x77000077,
		0xffffffff,
		0xffff0000,
		0xff00ff00
	};

	for (int level = 0; level < 11; level++)
	{
		D3DLOCKED_RECT rect;
		pTestTex->LockRect(level, &rect, NULL, 0);

		uint32_t* texdata = (uint32_t*)rect.pBits;

		int texsize = 1024 >> level;

		for (int i = 0; i < texsize * texsize; i++)
		{
			texdata[i] = colorlevels[level];
		}
	
		pTestTex->UnlockRect(level);
	}


	D3DXHANDLE hTest = g_pEffect9->GetParameterByName(NULL, "TestTexture");
	g_pEffect9->SetTexture(hTest, pTestTex);

	pd3dDevice->CreateTexture(1024, 1024, 0, 0, D3DFMT_A2R10G10B10,  D3DPOOL_MANAGED, &pFeedBackTex, NULL);

	pd3dDevice->CreateTexture(1024, 1024, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &pIndirectMap, NULL);

	int levelcount = pFeedBackTex->GetLevelCount();
	

	for ( uint64_t level = 0; level < 11; level++ )
	{
		D3DLOCKED_RECT rect;
		
		pFeedBackTex->LockRect(level, &rect, NULL, 0);

		uint32_t* ptexdata = (uint32_t*)rect.pBits;

		int mipsize = 1024 >> level;
		for (uint32_t j = 0; j < mipsize; j++)
			for(uint32_t i=0; i < mipsize; i++)
			{
				ptexdata[i + j * mipsize] = (level << 22) | (i << 12) | (j << 2);
			}
		
		pFeedBackTex->UnlockRect(level);

	}

	D3DXHANDLE hindirect = g_pEffect9->GetParameterByName(NULL, "indirectTex");
	g_pEffect9->SetTexture(hindirect, pFeedBackTex);

	for (int i = 0; i < 11; i++)
	{

		int texwidth = 1024 >> i;

		indirectTexData[i] = new uint32_t[texwidth*texwidth];

		memset(indirectTexData[i], 0xffffffff, sizeof(uint32_t)*texwidth*texwidth);

	}

	return S_OK;
}


int scale = 8;

//--------------------------------------------------------------------------------------
// Create any D3D9 resources that won't live through a device reset (D3DPOOL_DEFAULT) 
// or that are tied to the back buffer size 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9ResetDevice(IDirect3DDevice9* pd3dDevice,
	const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	HRESULT hr;

	V_RETURN(g_DialogResourceManager.OnD3D9ResetDevice());
	V_RETURN(g_SettingsDlg.OnD3D9ResetDevice());

	if (g_pFont9) V_RETURN(g_pFont9->OnResetDevice());
	if (g_pEffect9) V_RETURN(g_pEffect9->OnResetDevice());

	V_RETURN(D3DXCreateSprite(pd3dDevice, &g_pSprite9));
	g_pTxtHelper = new CDXUTTextHelper(g_pFont9, g_pSprite9, NULL, NULL, 15);

	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams(D3DX_PI / 4, fAspectRatio, 0.1f, 10000.0f);
	//g_Camera.SetWindow(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);

	g_HUD.SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
	g_HUD.SetSize(170, 170);
	g_SampleUI.SetLocation(pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 350);
	g_SampleUI.SetSize(170, 300);

	D3DXCreateTexture(pd3dDevice, pBackBufferSurfaceDesc->Width/ scale, pBackBufferSurfaceDesc->Height / scale, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A2R10G10B10, D3DPOOL_DEFAULT, &newRT0);
	D3DXCreateTexture(pd3dDevice, pBackBufferSurfaceDesc->Width / scale, pBackBufferSurfaceDesc->Height / scale, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A2R10G10B10, D3DPOOL_DEFAULT, &newRT1);
	D3DXCreateTexture(pd3dDevice, pBackBufferSurfaceDesc->Width / scale, pBackBufferSurfaceDesc->Height / scale, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A2R10G10B10, D3DPOOL_DEFAULT, &newRT2);

	D3DXCreateTexture(pd3dDevice, 1024, 1024, 0, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &indirectMap);

	for (int level = 0; level < 11; level++)
	{
		indirectMap->GetSurfaceLevel(level, &pIndirectRT[level]);
	}
	
	
	D3DXCreateTexture(pd3dDevice, pBackBufferSurfaceDesc->Width / scale, pBackBufferSurfaceDesc->Height / scale, 1, 0, D3DFMT_A2R10G10B10, D3DPOOL_SYSTEMMEM, &sysRT);



	
	
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	// Update the camera's position based on user input 
	g_Camera.FrameMove(fElapsedTime);
}



//--------------------------------------------------------------------------------------
// Render the scene using the D3D9 device
//--------------------------------------------------------------------------------------
int swit = 0;


void DrawQuad(IDirect3DDevice9* pDevice)
{
	struct QuadVertex
	{
		D3DXVECTOR3 pos;
		D3DXVECTOR2 uv;
	};

	QuadVertex v[4];

	v[0].pos = D3DXVECTOR3(-512.0f, 0.0f, -512.0f);
	v[0].uv = D3DXVECTOR2(0.0f, 1.0f);

	v[1].pos = D3DXVECTOR3(-512.0f, 0.0f, 512.0f);
	v[1].uv = D3DXVECTOR2(0.0f, 0.0f);

	v[2].pos = D3DXVECTOR3(512.0f, 0.0f, -512.0f);
	v[2].uv = D3DXVECTOR2(1.0f, 1.0f);

	v[3].pos = D3DXVECTOR3(512.0f, 0.0f, 512.0f);
	v[3].uv = D3DXVECTOR2(1.0f, 0.0f);


	pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v, sizeof(QuadVertex));
}

void DrawQuad(IDirect3DDevice9* pDevice,float x,float y,float quadsize)
{
	struct QuadVertex
	{
		D3DXVECTOR3 pos;
		D3DXVECTOR2 uv;
	};

	QuadVertex v[4];
	
	v[0].pos = D3DXVECTOR3(x- quadsize, 0.0f, y - quadsize);
	v[0].uv = D3DXVECTOR2(0.0f, 1.0f);

	v[1].pos = D3DXVECTOR3(x - quadsize, 0.0f, y + quadsize);
	v[1].uv = D3DXVECTOR2(0.0f, 0.0f);

	v[2].pos = D3DXVECTOR3(x + quadsize, 0.0f, y - quadsize);
	v[2].uv = D3DXVECTOR2(1.0f, 1.0f);

	v[3].pos = D3DXVECTOR3(x + quadsize, 0.0f, y + quadsize);
	v[3].uv = D3DXVECTOR2(1.0f, 0.0f);


	pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v, sizeof(QuadVertex));
}

struct Datastr
{
	int visit;
	int xbias;
	int ybias;
	int level;
	
};

Datastr pageindexVisit[1024];


struct PointVertex
{
	D3DXVECTOR3 pos;
	DWORD color;
};

PointVertex PointArray[10][512];
int vertexnum[10];

void updateIndirTex(IDirect3DDevice9* pdevice)
{
	g_pEffect9->BeginPass(5);
	pdevice->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE);
	for (int level = 9; level >= 0; level--)
	{
		for (int caculatelevel = 9; caculatelevel >= level; caculatelevel--)
		{
			if (vertexnum[caculatelevel] > 0)
			{
				pdevice->SetRenderTarget(0, pIndirectRT[caculatelevel]);
				pdevice->DrawPrimitiveUP(D3DPT_POINTLIST, vertexnum[caculatelevel], PointArray[caculatelevel], sizeof(PointVertex));
			}
		}

	}
	g_pEffect9->EndPass();
}

void InitProcessTex()
{
	static bool init = false;

	if (init == false)
	{
		for (int i = 0; i < 1024; i++)
		{			
			pageindexVisit[i].visit = -1;			
		}
			
		int pageindex = vtgen->getPageIndex();

		int xpage = pageindex % 32;
		int ypage = pageindex / 32;
		int compindex = 10 << 16 | (xpage << 8) | ypage;

		pageindexVisit[0].level = 10;
		pageindexVisit[0].xbias = 0;
		pageindexVisit[0].ybias = 0;
		pageindexVisit[0].visit = 0;

		int texadr = (10 << 24) | (0 + 0 * 4096);

		vtgen->updateTexture(pageindex, texadr);

		indirectTexData[10][0] = compindex;

		init = true;
	}
	
}


IDirect3DSurface9* psysSurf = NULL;
void ProcessFeedback(IDirect3DDevice9* pDevice)
{
	
	IDirect3DSurface9* pRT;
	IDirect3DSurface9* pOldRT;
	
	newRT0->GetSurfaceLevel(0, &pRT);
	pDevice->GetRenderTarget(0, &pOldRT);

	pDevice->SetRenderTarget(0, pRT);
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(255, 255, 255, 255), 1.0f, 0);

	g_pEffect9->BeginPass(2);
	terrainMesh->Renderlow();
	g_pEffect9->EndPass();

	pDevice->SetRenderTarget(0, pOldRT);

	if (psysSurf == nullptr)
	{
		sysRT->GetSurfaceLevel(0, &psysSurf);
	}

	for (int i = 0; i < 10; i++)
	{
		vertexnum[i] = 0;
	}

	pDevice->GetRenderTargetData(pRT, psysSurf);

	InitProcessTex();

	D3DSURFACE_DESC desc;
	psysSurf->GetDesc(&desc);

	D3DLOCKED_RECT rect;
	psysSurf->LockRect(&rect, NULL, D3DLOCK_READONLY);
	uint32_t* pfeedbackdata = (uint32_t *)rect.pBits;

	for (int i = 1; i < 1024; i++)
	{
		if (pageindexVisit[i].visit != -1)
		{
			pageindexVisit[i].visit++;
		}

		if (pageindexVisit[i].visit >= 3)
		{
			vtgen->recycleIndex(i);
			pageindexVisit[i].visit = -1;
			int level = pageindexVisit[i].level;
			int xbias = pageindexVisit[i].xbias;
			int ybias = pageindexVisit[i].ybias;
				
			int texsize = 1024 >> level;
			indirectTexData[level][xbias + ybias * texsize] = 0xffffffff;
				
			int& levelindex = vertexnum[level];
			PointArray[level][levelindex].pos = D3DXVECTOR3(xbias, ybias, level);
			PointArray[level][levelindex++].color = 0;
		}
	}

	int countindex = 0;
	for (int i = 0; i < desc.Width*desc.Height; i++)
	{
		if (pfeedbackdata[i] != 0xffffffff)
		{
			int level = pfeedbackdata[i] >> 22;
			int xbias = (pfeedbackdata[i] & 0x003ff000) >> 12;
			int ybias = (pfeedbackdata[i] & 0x00000ffc) >> 2;
			int texadr = (level << 24) | (xbias + ybias * 4096);

			int texsize = 1024 >> level;

			uint32_t& indirectData = indirectTexData[level][xbias + ybias * texsize];
			uint32_t oldlevel = indirectData >> 16;
			if (oldlevel != level)
			{
				int pageindex = vtgen->getPageIndex();

				if (pageindex == -1)
				{
					assert(0);
				}

				int xpage = pageindex % 32;
				int ypage = pageindex / 32;

				int compindex = level << 16 | (xpage << 8) | ypage;
				indirectData = compindex;

				pageindexVisit[pageindex].visit = 0;
				pageindexVisit[pageindex].level = level;
				pageindexVisit[pageindex].xbias = xbias;
				pageindexVisit[pageindex].ybias = ybias;

				vtgen->updateTexture(pageindex, texadr);

				int& levelindex = vertexnum[level];
				PointArray[level][levelindex].pos = D3DXVECTOR3(xbias, ybias, level);
				PointArray[level][levelindex++].color = compindex;
				countindex++;
			}
			else
			{
				int pageindex = ((indirectData & 0x0000ff00) >> 8) | (indirectData & 0x000000ff) << 5;
				pageindexVisit[pageindex].visit = 0;

			}
		}
	}
	
	psysSurf->UnlockRect();
		
	
	updateIndirTex(pDevice);
	

	pDevice->SetRenderTarget(0, pOldRT);
	SAFE_RELEASE(pRT);
	SAFE_RELEASE(pOldRT);
	
}

void CALLBACK OnD3D9FrameRender(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext)
{
	HRESULT hr;
	D3DXMATRIXA16 mWorld;
	D3DXMATRIXA16 mView;
	D3DXMATRIXA16 mProj;
	D3DXMATRIXA16 mWorldViewProjection;

	// If the settings dialog is being shown, then render it instead of rendering the app's scene
	if (g_SettingsDlg.IsActive())
	{
		g_SettingsDlg.OnRender(fElapsedTime);
		return;
	}

	// Clear the render target and the zbuffer 
	if (SUCCEEDED(pd3dDevice->BeginScene()))
	{
		// Get the projection & view matrix from the camera class
		mWorld = *g_Camera.GetWorldMatrix();
		mProj = *g_Camera.GetProjMatrix();
		mView = *g_Camera.GetViewMatrix();

		D3DXHANDLE tex = g_pEffect9->GetParameterByName(NULL, "g_HeightTexture");
		g_pEffect9->SetTexture(tex, terrainTex);

		mWorldViewProjection = mView * mProj;
		g_pEffect9->SetMatrix(g_hmWorldViewProjection, &mWorldViewProjection);

		g_pEffect9->Begin(nullptr, 0);

		ProcessFeedback(pd3dDevice);
		
		pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 45, 50, 170), 1.0f, 0);
		//update indirect tex

		D3DXHANDLE hheight = g_pEffect9->GetParameterByName(NULL, "indirectMap");
		g_pEffect9->SetTexture( hheight, indirectMap );

		D3DXHANDLE hcache = g_pEffect9->GetParameterByName(NULL, "CacheTexture");
		g_pEffect9->SetTexture(hcache, vtgen->getTex());
			
		g_pEffect9->BeginPass(3);
		terrainMesh->Renderlow();
		g_pEffect9->EndPass();

		g_pEffect9->End();


		DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats"); // These events are to help PIX identify what the code is doing
		RenderText();
		V(g_HUD.OnRender(fElapsedTime));
		V(g_SampleUI.OnRender(fElapsedTime));
		DXUT_EndPerfEvent();

		V(pd3dDevice->EndScene());
	}
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
	void* pUserContext)
{
	// Pass messages to dialog resource manager calls so GUI state is updated correctly
	*pbNoFurtherProcessing = g_DialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	// Pass messages to settings dialog if its active
	if (g_SettingsDlg.IsActive())
	{
		g_SettingsDlg.MsgProc(hWnd, uMsg, wParam, lParam);
		return 0;
	}

	// Give the dialogs a chance to handle the message first
	*pbNoFurtherProcessing = g_HUD.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;
	*pbNoFurtherProcessing = g_SampleUI.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	// Pass all remaining windows messages to camera so it can respond to user input
	g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

	if (uMsg == WM_LBUTTONDOWN)
	{
		swit++;
	}
	if (uMsg == WM_LBUTTONUP)
	{

	}

	return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	switch (nControlID)
	{
	case IDC_TOGGLEFULLSCREEN:
		DXUTToggleFullScreen(); break;
	case IDC_TOGGLEREF:
		DXUTToggleREF(); break;
	case IDC_CHANGEDEVICE:
		g_SettingsDlg.SetActive(!g_SettingsDlg.IsActive()); break;
	}
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9LostDevice(void* pUserContext)
{
	g_DialogResourceManager.OnD3D9LostDevice();
	g_SettingsDlg.OnD3D9LostDevice();
	if (g_pFont9) g_pFont9->OnLostDevice();
	if (g_pEffect9) g_pEffect9->OnLostDevice();
	SAFE_RELEASE(g_pSprite9);
	SAFE_DELETE(g_pTxtHelper);

	SAFE_RELEASE(newRT0);
	SAFE_RELEASE(newRT1);
	SAFE_RELEASE(sysRT);
	SAFE_RELEASE(psysSurf);

}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9CreateDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9DestroyDevice(void* pUserContext)
{
	g_DialogResourceManager.OnD3D9DestroyDevice();
	g_SettingsDlg.OnD3D9DestroyDevice();

	SAFE_RELEASE(g_pEffect9);
	SAFE_RELEASE(g_pFont9);

	SAFE_RELEASE(terrainTex);


	terrainMesh->Shut();
	delete terrainMesh;
	vtgen->shutdown();
	delete vtgen;

	SAFE_RELEASE(pTestTex);

	SAFE_RELEASE(pFeedBackTex);

	


}


