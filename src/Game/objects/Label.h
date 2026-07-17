//
// Created by karp on 14/04/2026.
//

#ifndef CC_GAME_LABEL_H
#define CC_GAME_LABEL_H

#include <string>
#include <memory>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Drawable.h"

namespace Game {

enum class HorizontalAlignment { Left, Center, Right };
enum class VerticalAlignment { Top, Middle, Bottom };

class Label : public Drawable {
public:
    Label(UnitBounds bounds, std::shared_ptr<TTF_Font> font, std::string text = "",
          SDL_Color color = {.r = 255, .g = 255, .b = 255, .a = 255});
    ~Label() override = default;

    void Update() override {}
    void Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) override;

    void SetText(std::string_view newText);
    void SetColor(SDL_Color newColor);
    void SetAlignment(HorizontalAlignment h, VerticalAlignment v);

private:
    std::string text;
    SDL_Color color;
    std::shared_ptr<TTF_Font> font;
    std::shared_ptr<SDL_Texture> textTexture;
    
    HorizontalAlignment hAlign = HorizontalAlignment::Left;
    VerticalAlignment vAlign = VerticalAlignment::Top;

    int textW = 0;
    int textH = 0;
    bool dirty = true;

    void RebuildTexture(SDL_Renderer* renderer);
};

} // namespace Game

#endif //CC_GAME_LABEL_H
