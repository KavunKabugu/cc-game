#ifndef CC_GAME_INPUT_FIELD_H
#define CC_GAME_INPUT_FIELD_H

#include <functional>
#include <memory>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "Drawable.h"
#include "Game/Events/Interfaces.h"

namespace Game {

class InputField final : public Drawable,
                         public IMouseClickable,
                         public IMouseHoverable,
                         public IKeyHandler,
                         public ITextInputHandler {
public:
    InputField(
        UnitBounds bounds,
        std::shared_ptr<TTF_Font> font,
        std::string initialText = "",
        std::size_t maxLength = 24);

    void Update() override {}
    void Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) override;

    IMouseClickable* AsMouseClickable() override { return this; }
    IMouseHoverable* AsMouseHoverable() override { return this; }
    IKeyHandler* AsKeyHandler() override { return this; }
    ITextInputHandler* AsTextInputHandler() override { return this; }

    bool OnMouseButtonDown(int button, UnitPoint localPos) override;
    bool OnMouseButtonUp(int button, UnitPoint localPos) override;
    void OnMouseButtonClicked(int button, UnitPoint localPos) override;
    void OnMouseEnter() override;
    void OnMouseLeave() override;
    void OnMouseMove(UnitPoint localPos) override;

    bool OnKeyDown(SDL_Keycode key, Uint64 timestamp) override;
    bool OnKeyUp(SDL_Keycode key, Uint64 timestamp) override;

    bool OnTextInput(const char* text) override;
    void OnTextInputFocusLost() override;

    void SetText(std::string newText);
    [[nodiscard]] const std::string& GetText() const { return text; }
    void SetMaxLength(std::size_t maxLength);
    void SetOnChanged(std::function<void(const std::string&)> callback);
    void SetColors(SDL_Color normal, SDL_Color focused, SDL_Color textCol);

    void Focus();
    void Blur(bool notifyChanged = true);
    [[nodiscard]] bool IsFocused() const { return focused; }

private:
    void RebuildTextTexture(SDL_Renderer* renderer);
    void InsertUtf8(std::string_view utf8);
    void DeleteBackward();

    std::shared_ptr<TTF_Font> font;
    std::string text;
    std::shared_ptr<SDL_Texture> textTexture;
    std::function<void(const std::string&)> onChanged;

    SDL_Color normalColor{.r = 40, .g = 48, .b = 64, .a = 255};
    SDL_Color focusedColor{.r = 55, .g = 70, .b = 95, .a = 255};
    SDL_Color textColor{.r = 245, .g = 245, .b = 245, .a = 255};

    std::size_t maxLength = 24;
    int textW = 0;
    int textH = 0;
    bool textDirty = true;
    bool focused = false;
    bool hovered = false;
    bool pressed = false;
    std::string textAtFocusStart;
};

} // namespace Game

#endif // CC_GAME_INPUT_FIELD_H
