#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "../../Controller/Controller.hpp"
#include <wincodec.h>
#include "../../Textures/controller_glyphs_png.hpp"

// ===========================================================
//  SDLGamepadSupport (Button Glyphs)
// ===========================================================

static constexpr wchar_t GLYPH_SPACE = 0x00A0;
static constexpr int GLYPH_COLUMNS = 16;
static constexpr int GLYPH_ROWS = 16;
static constexpr int GLYPH_CELL_SIZE = 64;
static constexpr float GLYPH_SCALE = 1.45f;
static constexpr float MENU_GLYPH_SCALE = 1.8f;
static constexpr int SCREEN_ID_MAIN = 1;

struct GlyphVertex
{
	float x, y, z;
	uint32_t color;
	float u, v;
};

struct GlyphRun
{
	int start;
	int count;
	int cell;
};

struct GlyphString
{
	std::wstring text;
	std::wstring resolvedText;
	std::vector<GlyphRun> runs;
	BYTE font[0x28];
	int spaceCount = 4;
	float scale = GLYPH_SCALE;
	int wrapWidth = 0;
	int version = 0;
	GamepadGlyphs glyphs = GamepadGlyphs::Count;
};

using CreateDynamicTexture_t = int(__thiscall*)(int, int*, void*);
using TextureData_t = void(__thiscall*)(void*);
using ReadDDSTextureData_t = bool(__cdecl*)(void*, void*);
using SetDynamicTexture_t = int(__thiscall*)(int, int);
using DrawPrimGT4_t = int(__thiscall*)(int, GlyphVertex*, uint32_t);

static std::unordered_map<int, GlyphString> glyphStrings;
static int glyphTexture = 0;
static bool isGlyphTextureCreated = false;
static int glyphsVersion = 0;
static int menuPromptsString = 0;
static float menuPromptsAnchor[2] = {};
static bool isMenuPromptsVisible = false;
static bool isHelpTextFitted = false;

class GlyphTextureStream
{
public:
	GlyphTextureStream(const uint8_t* data, size_t size) : m_data(data), m_size(size) {}

	virtual void Release() {}

	virtual uint32_t Read(void* pData, uint32_t size)
	{
		if (m_pos + size > m_size)
		{
			memset(pData, 0, size);
			m_hasError = true;
			return 1;
		}

		memcpy(pData, m_data + m_pos, size);
		m_pos += size;
		return 0;
	}

	virtual uint32_t ReadString(char* pData, uint32_t size) { return 1; }
	virtual bool HasErrorOccurred() { return m_hasError; }
	virtual bool CanSeek() { return true; }

	virtual uint32_t SeekTo(uint64_t offset)
	{
		if (offset > m_size) return 1;

		m_pos = (size_t)offset;
		return 0;
	}

	virtual uint64_t GetPos() { return m_pos; }
	virtual uint64_t GetLen() { return m_size; }
	virtual ~GlyphTextureStream() = default;

private:
	const uint8_t* m_data;
	size_t m_size;
	size_t m_pos = 0;
	bool m_hasError = false;
};

// Decodes the glyphs into an uncompressed DDS with Windows Imaging Component
static bool DecodeGlyphImage(std::vector<uint8_t>& dds)
{
	const UINT width = GLYPH_COLUMNS * GLYPH_CELL_SIZE;
	const UINT height = GLYPH_ROWS * GLYPH_CELL_SIZE;
	const DWORD header[32] = { 0x20534444, 124, 0x100F, height, width, width * 4, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0x41, 0, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, 0x1000, 0, 0, 0, 0 };

	dds.resize(sizeof(header) + width * height * 4);
	memcpy(dds.data(), header, sizeof(header));

	HRESULT hrCom = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	bool isDecoded = false;

	IWICImagingFactory* factory = nullptr;
	IWICStream* stream = nullptr;
	IWICBitmapDecoder* decoder = nullptr;
	IWICBitmapFrameDecode* frame = nullptr;
	IWICFormatConverter* converter = nullptr;
	UINT frameWidth = 0, frameHeight = 0;

	if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory1, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))) &&
		SUCCEEDED(factory->CreateStream(&stream)) &&
		SUCCEEDED(stream->InitializeFromMemory((BYTE*)ControllerGlyphs_png, (DWORD)ControllerGlyphs_png_len)) &&
		SUCCEEDED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder)) &&
		SUCCEEDED(decoder->GetFrame(0, &frame)) &&
		SUCCEEDED(frame->GetSize(&frameWidth, &frameHeight)) && frameWidth == width && frameHeight == height &&
		SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
		SUCCEEDED(converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)) &&
		SUCCEEDED(converter->CopyPixels(nullptr, width * 4, width * height * 4, dds.data() + sizeof(header))))
	{
		isDecoded = true;
	}

	if (converter) converter->Release();
	if (frame) frame->Release();
	if (decoder) decoder->Release();
	if (stream) stream->Release();
	if (factory) factory->Release();
	if (SUCCEEDED(hrCom)) CoUninitialize();

	return isDecoded;
}

static int GetGlyphTexture()
{
	if (!isGlyphTextureCreated)
	{
		isGlyphTextureCreated = true;

		std::vector<uint8_t> dds;
		int* pTexMgr = (int*)ScreenHelper::TextureMgr_Addr;
		if (pTexMgr && *pTexMgr && DecodeGlyphImage(dds))
		{
			int texMgr = *pTexMgr;
			int texMgrVtable = *(int*)texMgr;

			GlyphTextureStream stream(dds.data(), dds.size());

			if (g_State.CurrentFEARGame == FEARXP)
			{
				auto textureDataConstructor = reinterpret_cast<TextureData_t>(GetAddress(Addr::TextureDataConstructor));
				auto textureDataDestructor = reinterpret_cast<TextureData_t>(GetAddress(Addr::TextureDataDestructor));
				auto readDDSTextureData = reinterpret_cast<ReadDDSTextureData_t>(GetAddress(Addr::ReadDDSTextureData));

				alignas(8) BYTE textureData[0x100];
				textureDataConstructor(textureData);

				if (readDDSTextureData(&stream, textureData))
				{
					CreateDynamicTexture_t createDynamicTexture = (CreateDynamicTexture_t)(*(int*)(texMgrVtable + 0x14));
					createDynamicTexture(texMgr, &glyphTexture, textureData);
				}

				textureDataDestructor(textureData);
			}
			else
			{
				CreateDynamicTexture_t createDynamicTexture = (CreateDynamicTexture_t)(*(int*)(texMgrVtable + 0x18));
				createDynamicTexture(texMgr, &glyphTexture, &stream);
			}
		}
	}

	return glyphTexture;
}

// Cell of the glyph of a button on the controller in use, -1 when it has none
static int GetGlyphCell(int button)
{
	if (button < 0 || button >= (int)std::size(ControllerGlyphs_cells[0]) || !GetGlyphTexture())
		return -1;

	uint8_t cell = ControllerGlyphs_cells[(int)GetCapabilities().glyphs][button];
	return (cell != 0xFF) ? cell : -1;
}

static bool HasPromptChars(const wchar_t* text)
{
	for (; text && *text; text++)
	{
		if (*text >= COMMAND_PROMPT_CHAR && *text < TRIGGER_GLYPH_CHAR + 2)
			return true;
	}

	return false;
}

static void AppendResolvedChar(GlyphString& glyphString, wchar_t c)
{
	if (c < BUTTON_GLYPH_CHAR || c >= TRIGGER_GLYPH_CHAR + 2)
	{
		glyphString.resolvedText += c;
		return;
	}

	int cell = GetGlyphCell(c - BUTTON_GLYPH_CHAR);
	if (cell >= 0)
	{
		glyphString.runs.push_back({ (int)glyphString.resolvedText.size(), glyphString.spaceCount, cell });
		glyphString.resolvedText.append(glyphString.spaceCount, GLYPH_SPACE);
		return;
	}

	// The name of the button when there is no glyph
	bool isTrigger = c >= TRIGGER_GLYPH_CHAR;
	glyphString.resolvedText += GetGamepadInputName(isTrigger, c - (isTrigger ? TRIGGER_GLYPH_CHAR : BUTTON_GLYPH_CHAR), true);
}

// The text with the keys or the buttons of its commands and strings, for the input in use
static void ResolveGlyphString(GlyphString& glyphString)
{
	glyphString.resolvedText.clear();
	glyphString.runs.clear();
	glyphString.glyphs = GetCapabilities().glyphs;

	for (wchar_t c : glyphString.text)
	{
		if (c >= COMMAND_PROMPT_CHAR && c <= COMMAND_PROMPT_CHAR + MAX_COMMAND_ID)
		{
			if (!userProfile) continue;

			int commandId = c - COMMAND_PROMPT_CHAR;
			const wchar_t* name = ShouldShowControllerPrompts() ? GetControllerTriggerName(userProfile, commandId) : GetTriggerNameFromCommandID(userProfile, commandId);

			for (; *name; name++)
			{
				AppendResolvedChar(glyphString, *name);
			}
		}
		else if (c >= INPUT_STRING_CHAR && c < INPUT_STRING_CHAR + std::size(inputStrings))
		{
			const char* id = inputStrings[c - INPUT_STRING_CHAR];
			const wchar_t* text = ShouldShowControllerPrompts() ? GetControllerString(HashHelper::FNV1aRuntime(id)) : nullptr;

			if (!text && LoadGameString && stringDatabase)
				text = LoadGameString(stringDatabase, (char*)id);

			for (; text && *text; text++)
			{
				AppendResolvedChar(glyphString, *text);
			}
		}
		else
		{
			AppendResolvedChar(glyphString, c);
		}
	}
}

static bool FitGlyphSpaces(int textureString, int hString, GlyphString& glyphString)
{
	if (glyphString.runs.empty()) return false;

	int placementRect[4], blackBox[4];
	float uvPos[2], uvDims[2];
	if (TextureStringGetCharRect(textureString, hString, glyphString.runs[0].start, placementRect, blackBox, uvPos, uvDims) != 0)
		return false;

	int spaceWidth = placementRect[2] - placementRect[0];
	int glyphSize = (int)((placementRect[3] - placementRect[1]) * glyphString.scale);
	if (spaceWidth <= 0) return false;

	int spaceCount = std::clamp((glyphSize + spaceWidth - 1) / spaceWidth, 1, 16);
	if (spaceCount == glyphString.spaceCount) return false;

	glyphString.spaceCount = spaceCount;
	ResolveGlyphString(glyphString);
	return true;
}

static int WordWrapGlyphString(int textureString, int hString, const GlyphString& glyphString)
{
	wchar_t* chars = *(wchar_t**)(hString + 0x24);
	size_t length = glyphString.resolvedText.size();
	bool isResolvedText = chars && *(uint32_t*)(hString + 0x20) == length && wmemcmp(chars, glyphString.resolvedText.c_str(), length) == 0;

	if (isResolvedText)
	{
		for (const GlyphRun& run : glyphString.runs)
		{
			wmemset(chars + run.start, L'_', run.count);
		}
	}

	int result = TextureStringWordWrap(textureString, hString, glyphString.wrapWidth);

	if (isResolvedText)
	{
		for (const GlyphRun& run : glyphString.runs)
		{
			wmemset(chars + run.start, GLYPH_SPACE, run.count);
		}
	}

	return result;
}

static void CreateGlyphString(int textureString, int hString, GlyphString& glyphString)
{
	if (FitGlyphSpaces(textureString, hString, glyphString))
		TextureStringRecreate(textureString, hString, glyphString.resolvedText.c_str(), (int)glyphString.font);

	if (glyphString.wrapWidth != 0)
		WordWrapGlyphString(textureString, hString, glyphString);
}
static GlyphString* GetGlyphString(int textureString, int hString)
{
	auto it = glyphStrings.find(hString);
	if (it == glyphStrings.end())
		return nullptr;

	GlyphString& glyphString = it->second;

	uint32_t length = 0;
	std::wstring text;
	if (TextureStringGetStringLength(textureString, hString, &length) == 0 && length == glyphString.resolvedText.size())
	{
		text.resize(length + 1);
		TextureStringGetString(textureString, hString, &text[0], length + 1);
		text.resize(length);
	}

	if (text != glyphString.resolvedText)
	{
		glyphStrings.erase(it);
		return nullptr;
	}

	if (glyphString.version != glyphsVersion || glyphString.glyphs != GetCapabilities().glyphs)
	{
		glyphString.version = glyphsVersion;
		ResolveGlyphString(glyphString);

		if (glyphString.resolvedText != text)
		{
			TextureStringRecreate(textureString, hString, glyphString.resolvedText.c_str(), (int)glyphString.font);
			CreateGlyphString(textureString, hString, glyphString);
		}
	}

	return glyphString.runs.empty() ? nullptr : &glyphString;
}

static void RenderGlyphs(int textureString, int hString, int pDrawPrim, const GlyphString& glyphString, const float* vAnchor, uint32_t nColor, const float* vAnchorScale, const float* vGround, const float* vDown, const float* vStretch, bool bSnapAnchor, const int* rClipRect)
{
	int texture = GetGlyphTexture();
	if (!texture) return;

	int extents[4];
	if (TextureStringGetStringExtents(textureString, hString, extents) != 0)
		return;

	float offsetX = (extents[2] - extents[0]) * vAnchorScale[0];
	float offsetY = (extents[3] - extents[1]) * vAnchorScale[1];

	if (bSnapAnchor)
	{
		offsetX = floorf(offsetX);
		offsetY = floorf(offsetY);
	}

	uint32_t color = (nColor & 0x00FFFFFF) ? (nColor | 0x00FFFFFF) : nColor;

	std::vector<GlyphVertex> vertices;
	vertices.reserve(glyphString.runs.size() * 4);

	for (const GlyphRun& run : glyphString.runs)
	{
		int first[4], last[4], blackBox[4];
		float uvPos[2], uvDims[2];
		if (TextureStringGetCharRect(textureString, hString, run.start, first, blackBox, uvPos, uvDims) != 0 ||
			TextureStringGetCharRect(textureString, hString, run.start + run.count - 1, last, blackBox, uvPos, uvDims) != 0)
			continue;

		float size = (first[3] - first[1]) * glyphString.scale;
		float x = (first[0] + last[2]) * 0.5f - offsetX;
		float y = (first[1] + first[3]) * 0.5f - offsetY;
		float centerX = vAnchor[0] + x * vGround[0] + y * vDown[0];
		float centerY = vAnchor[1] + x * vGround[1] + y * vDown[1];
		float halfWidth = size * 0.5f * vStretch[0];
		float halfHeight = size * 0.5f * vStretch[1];

		float u0 = (float)(run.cell % GLYPH_COLUMNS) / GLYPH_COLUMNS;
		float v0 = (float)(run.cell / GLYPH_COLUMNS) / GLYPH_ROWS;
		float u1 = u0 + 1.0f / GLYPH_COLUMNS;
		float v1 = v0 + 1.0f / GLYPH_ROWS;

		float left = -halfWidth, right = halfWidth, top = -halfHeight, bottom = halfHeight;

		// Clipped strings are never rotated
		if (rClipRect)
		{
			float width = right - left, height = bottom - top;
			float clipLeft = std::max(left, rClipRect[0] - centerX);
			float clipRight = std::min(right, rClipRect[2] - centerX);
			float clipTop = std::max(top, rClipRect[1] - centerY);
			float clipBottom = std::min(bottom, rClipRect[3] - centerY);
			if (clipLeft >= clipRight || clipTop >= clipBottom)
				continue;

			float uWidth = u1 - u0, vHeight = v1 - v0;
			u1 = u0 + uWidth * (clipRight - left) / width;
			u0 += uWidth * (clipLeft - left) / width;
			v1 = v0 + vHeight * (clipBottom - top) / height;
			v0 += vHeight * (clipTop - top) / height;
			left = clipLeft; right = clipRight; top = clipTop; bottom = clipBottom;
		}

		auto AddVertex = [&](float across, float down, float u, float v)
		{
			float vertexX = centerX + across * vGround[0] + down * vDown[0] - 0.5f;
			float vertexY = centerY + across * vGround[1] + down * vDown[1] - 0.5f;
			vertices.push_back({ vertexX, vertexY, 1.0f, color, u, v });
		};

		AddVertex(left, top, u0, v0);
		AddVertex(right, top, u1, v0);
		AddVertex(right, bottom, u1, v1);
		AddVertex(left, bottom, u0, v1);
	}

	if (vertices.empty()) return;

	int drawPrimVtable = *(int*)pDrawPrim;
	SetDynamicTexture_t setDynamicTexture = (SetDynamicTexture_t)(*(int*)(drawPrimVtable + 0x20));
	DrawPrimGT4_t drawPrimGT4 = (DrawPrimGT4_t)(*(int*)(drawPrimVtable + 0x44));

	setDynamicTexture(pDrawPrim, texture);
	drawPrimGT4(pDrawPrim, vertices.data(), (uint32_t)(vertices.size() / 4));
	setDynamicTexture(pDrawPrim, 0);
}

static int __fastcall TextureStringCreate_Hook(int thisPtr, int, const wchar_t* pszString, int font)
{
	if (!HasPromptChars(pszString))
	{
		int hString = TextureStringCreate(thisPtr, pszString, font);
		glyphStrings.erase(hString);
		return hString;
	}

	GlyphString glyphString;
	glyphString.text = pszString;
	glyphString.version = glyphsVersion;
	memcpy(glyphString.font, (void*)font, sizeof(glyphString.font));
	ResolveGlyphString(glyphString);

	int hString = TextureStringCreate(thisPtr, glyphString.resolvedText.c_str(), font);
	if (hString == 0)
		return 0;

	CreateGlyphString(thisPtr, hString, glyphString);
	glyphStrings[hString] = glyphString;
	return hString;
}

static int __fastcall TextureStringWordWrap_Hook(int thisPtr, int, int hString, int nWidth)
{
	auto it = glyphStrings.find(hString);
	if (it == glyphStrings.end())
		return TextureStringWordWrap(thisPtr, hString, nWidth);

	it->second.wrapWidth = nWidth;
	return WordWrapGlyphString(thisPtr, hString, it->second);
}

static int __fastcall TextureStringRecreate_Hook(int thisPtr, int, int hString, const wchar_t* pszString, int font)
{
	if (!HasPromptChars(pszString))
	{
		glyphStrings.erase(hString);
		return TextureStringRecreate(thisPtr, hString, pszString, font);
	}

	GlyphString glyphString;
	glyphString.text = pszString;
	glyphString.version = glyphsVersion;
	memcpy(glyphString.font, (void*)font, sizeof(glyphString.font));
	ResolveGlyphString(glyphString);

	int result = TextureStringRecreate(thisPtr, hString, glyphString.resolvedText.c_str(), font);
	if (result != 0)
	{
		glyphStrings.erase(hString);
		return result;
	}

	CreateGlyphString(thisPtr, hString, glyphString);
	glyphStrings[hString] = glyphString;
	return result;
}

static int __fastcall TextureStringRender_Hook(int thisPtr, int, int hString, int pDrawPrim, const float* vAnchor, uint32_t nColor, const float* vAnchorScale, const float* vGround, const float* vDown, const float* vStretch, bool bSnapAnchor)
{
	GlyphString* glyphString = GetGlyphString(thisPtr, hString);
	int result = TextureStringRender(thisPtr, hString, pDrawPrim, vAnchor, nColor, vAnchorScale, vGround, vDown, vStretch, bSnapAnchor);

	if (glyphString)
		RenderGlyphs(thisPtr, hString, pDrawPrim, *glyphString, vAnchor, nColor, vAnchorScale, vGround, vDown, vStretch, bSnapAnchor, nullptr);

	return result;
}

static int __fastcall TextureStringRenderClipped_Hook(int thisPtr, int, int hString, int pDrawPrim, const int* rClipRect, const float* vAnchor, uint32_t nColor, const float* vAnchorScale, bool bSnapAnchor)
{
	GlyphString* glyphString = GetGlyphString(thisPtr, hString);
	int result = TextureStringRenderClipped(thisPtr, hString, pDrawPrim, rClipRect, vAnchor, nColor, vAnchorScale, bSnapAnchor);

	if (glyphString)
	{
		const float vGround[2] = { 1.0f, 0.0f };
		const float vDown[2] = { 0.0f, 1.0f };
		const float vStretch[2] = { 1.0f, 1.0f };
		RenderGlyphs(thisPtr, hString, pDrawPrim, *glyphString, vAnchor, nColor, vAnchorScale, vGround, vDown, vStretch, bSnapAnchor, rClipRect);
	}

	return result;
}

void RefreshButtonGlyphs()
{
	glyphsVersion++;
}

// ===========================================================
//  SDLGamepadSupport (Menu Prompts)
// ===========================================================

static const wchar_t* LoadMenuPromptString(const char* id, const wchar_t* defaultText)
{
	if (!LoadGameString || !stringDatabase)
		return defaultText;

	return LoadGameString_Hook(stringDatabase, (char*)id);
}

static float UpdateMenuPrompts(int screen)
{
	isMenuPromptsVisible = false;

	if (!ShouldShowControllerPrompts() || g_State.isMsgBoxVisible)
		return (float)g_State.currentWidth;

	SDL_GamepadButton backButton = GetGamepadMenuButton(false);
	SDL_GamepadButton selectButton = GetGamepadMenuButton(true);
	wchar_t backGlyph = GetGamepadButtonGlyph(false, backButton);
	wchar_t selectGlyph = GetGamepadButtonGlyph(false, selectButton);
	int textureString = MemoryHelper::ReadMemory<int>(g_State.textureStringAddr);
	int drawPrim = MemoryHelper::ReadMemory<int>(g_State.drawPrimAddr);
	if (!backGlyph || !selectGlyph || !textureString || !drawPrim || GetGlyphCell(backButton) < 0 || GetGlyphCell(selectButton) < 0)
		return (float)g_State.currentWidth;

	int screenMgr = *(int*)(screen + 0xC);
	int helpString = screenMgr + 0x160;

	std::wstring text;

	if (g_State.maxCurrentType > 0)
	{
		text += GetGamepadButtonGlyph(false, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
		text += GetGamepadButtonGlyph(false, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
		text += L' ';
		text += LoadMenuPromptString("IDS_CATEGORY", L"Category");
		text += L"    ";
	}

	if (*(bool*)(screen + 0x110))
	{
		text += backGlyph;
		text += L' ';
		text += LoadMenuPromptString("IDS_BACK", L"Back");
		text += L"    ";
	}

	text += selectGlyph;
	text += L' ';
	text += LoadMenuPromptString("IDS_SELECT", L"Select");

	auto it = glyphStrings.find(menuPromptsString);
	if (menuPromptsString == 0)
	{
		menuPromptsString = TextureStringCreate_Hook(textureString, 0, text.c_str(), helpString);
	}
	else if (it == glyphStrings.end() || it->second.text != text || memcmp(it->second.font, (void*)helpString, sizeof(it->second.font)) != 0)
	{
		TextureStringRecreate_Hook(textureString, 0, menuPromptsString, text.c_str(), helpString);
	}

	if (menuPromptsString == 0)
		return (float)g_State.currentWidth;

	it = glyphStrings.find(menuPromptsString);
	if (it != glyphStrings.end() && it->second.scale != MENU_GLYPH_SCALE)
	{
		it->second.scale = MENU_GLYPH_SCALE;
		CreateGlyphString(textureString, menuPromptsString, it->second);
	}

	GetGlyphString(textureString, menuPromptsString);

	int extents[4];
	if (TextureStringGetStringExtents(textureString, menuPromptsString, extents) != 0)
		return (float)g_State.currentWidth;

	menuPromptsAnchor[0] = g_State.currentWidth - *(int*)(screenMgr + 0x22C) * (g_State.currentWidth / 640.0f);
	menuPromptsAnchor[1] = *(int*)(screenMgr + 0x1F0) * (g_State.currentHeight / 480.0f);

	if (*(int*)(screen + 0x10) == SCREEN_ID_MAIN)
		menuPromptsAnchor[1] -= *(uint32_t*)(helpString + 0x20) * 0.5f;

	isMenuPromptsVisible = true;
	return menuPromptsAnchor[0] - (extents[2] - extents[0]);
}

static void RenderMenuPrompts(int screen)
{
	if (!isMenuPromptsVisible)
		return;

	int textureString = MemoryHelper::ReadMemory<int>(g_State.textureStringAddr);
	int drawPrim = MemoryHelper::ReadMemory<int>(g_State.drawPrimAddr);
	int helpString = *(int*)(screen + 0xC) + 0x160;
	const float anchorScale[2] = { 1.0f, 0.0f };
	const float ground[2] = { 1.0f, 0.0f };
	const float down[2] = { 0.0f, 1.0f };
	const float stretch[2] = { 1.0f, 1.0f };

	TextureStringSetupTextRendering(textureString, drawPrim);
	TextureStringRender_Hook(textureString, 0, menuPromptsString, drawPrim, menuPromptsAnchor, *(uint32_t*)(helpString + 0x58), anchorScale, ground, down, stretch, true);
}

static void FitHelpText(int screen, float promptsLeft)
{
	int screenMgr = *(int*)(screen + 0xC);
	int helpString = screenMgr + 0x160;
	float xRatio = g_State.currentWidth / 640.0f;
	float yRatio = g_State.currentHeight / 480.0f;
	float helpLeft = *(int*)(screenMgr + 0x1EC) * xRatio;
	float helpTop = *(int*)(screenMgr + 0x1F0) * yRatio;
	uint32_t helpWidth = (uint32_t)(*(uint16_t*)(screenMgr + 0x228) * xRatio);
	uint32_t fontHeight = *(uint32_t*)(helpString + 0x20);

	bool isOverlapping = helpLeft + helpWidth + fontHeight > promptsLeft;
	if (!isOverlapping && !isHelpTextFitted)
		return;

	isHelpTextFitted = isOverlapping;

	int textureString = MemoryHelper::ReadMemory<int>(g_State.textureStringAddr);
	int hString = *(int*)(helpString + 0x28);

	// CLTGUIString::WordWrap
	uint32_t width = isOverlapping ? (uint32_t)std::max(promptsLeft - helpLeft - fontHeight, 0.0f) : helpWidth;
	if (width <= fontHeight)
		width = 0;

	if (*(uint32_t*)(helpString + 0x4C) != width)
	{
		*(uint32_t*)(helpString + 0x4C) = width;

		if (textureString && hString)
			TextureStringWordWrap_Hook(textureString, 0, hString, width);
	}

	float top = helpTop;
	int extents[4];
	if (isOverlapping && textureString && hString && TextureStringGetStringExtents(textureString, hString, extents) == 0 && fontHeight != 0)
	{
		float overflow = helpTop + (extents[3] - extents[1]) - g_State.currentHeight;
		if (overflow > 0.0f)
			top -= ceilf(overflow / fontHeight) * fontHeight;
	}

	*(float*)(helpString + 0x64) = top;
}

static bool __fastcall BaseScreenRender_Hook(int thisPtr, int)
{
	if (g_State.currentWidth == 0)
		return BaseScreenRender(thisPtr);

	float promptsLeft = UpdateMenuPrompts(thisPtr);
	FitHelpText(thisPtr, promptsLeft);

	bool result = BaseScreenRender(thisPtr);
	RenderMenuPrompts(thisPtr);

	return result;
}
