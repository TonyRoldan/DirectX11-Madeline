#ifndef UI_H
#define UI_H

#include <SpriteFont.h>

namespace MAD
{
	struct RenderSprite{};
	struct RenderText{};

	struct LogoScreen {};
	struct DirectXScreen {};
	struct GatewareScreen {};
	struct FlecsScreen {};
	struct TitleScreen {};
	struct MainMenu {};
	struct PlayGame {};
	struct WaveScreen {};
	struct GameOverScreen {};
	struct PauseGame {};
	struct HighScores {};
	struct Credits {};

	struct SpriteCount { unsigned int numSprites; };
	struct SpriteOffsets { DirectX::SimpleMath::Vector2 values; };
		
	struct TextColor { DirectX::XMVECTOR value; };
	struct FontType { DirectX::SpriteFont* type; };
	
	struct Vec2 { DirectX::SimpleMath::Vector2 value; };
	struct Dimensions { unsigned x; unsigned y; };
	struct Float { float value; };

	struct Left {};
	struct Right{};
	struct Center{};
	struct Top{};

	struct SpriteProperties { SpriteCount spriteCount; Dimensions size; Vec2 pos; Vec2 origin; Float origScale; Float newScale; Float alpha; };
	struct TextProperties { FontType font; TextColor color; Dimensions size; Vec2 origPos; Vec2 newPos; Vec2 origin; Float origScale; Float newScale; Float alpha; };

	struct Sprite { Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view; SpriteProperties props; };
	struct Text { std::wstring value; TextProperties props; };
}

#endif