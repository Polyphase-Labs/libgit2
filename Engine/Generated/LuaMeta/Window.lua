--- @meta

---@class Window : Widget
Window = {}

---@param id string
function Window:SetWindowId(id) end

---@return string
function Window:GetWindowId() end

---@param title string
function Window:SetTitle(title) end

---@return string
function Window:GetTitle() end

---@param alignment integer
function Window:SetTitleAlignment(alignment) end

---@return integer
function Window:GetTitleAlignment() end

---@param size number
function Window:SetTitleFontSize(size) end

---@return number
function Window:GetTitleFontSize() end

---@param offset Vector
function Window:SetTitleOffset(offset) end

---@return Vector
function Window:GetTitleOffset() end

---@param left number
---@param top number
---@param right number
---@param bottom number
function Window:SetContentPadding(left, top, right, bottom) end

---@param left number
function Window:SetContentPaddingLeft(left) end

---@param top number
function Window:SetContentPaddingTop(top) end

---@param right number
function Window:SetContentPaddingRight(right) end

---@param bottom number
function Window:SetContentPaddingBottom(bottom) end

---@return number
function Window:GetContentPaddingLeft() end

---@return number
function Window:GetContentPaddingTop() end

---@return number
function Window:GetContentPaddingRight() end

---@return number
function Window:GetContentPaddingBottom() end

function Window:Show() end

function Window:Hide() end

function Window:Close() end

---@param hidden boolean
function Window:SetStartHidden(hidden) end

---@return boolean
function Window:GetStartHidden() end

---@param draggable boolean
function Window:SetDraggable(draggable) end

---@return boolean
function Window:IsDraggable() end

---@param resizable boolean
function Window:SetResizable(resizable) end

---@return boolean
function Window:IsResizable() end

---@param show boolean
function Window:SetShowCloseButton(show) end

---@return boolean
function Window:GetShowCloseButton() end

---@param show boolean
function Window:SetShowTitleBar(show) end

---@return boolean
function Window:GetShowTitleBar() end

---@param height number
function Window:SetTitleBarHeight(height) end

---@return number
function Window:GetTitleBarHeight() end

---@param minSize Vector
function Window:SetMinSize(minSize) end

---@return Vector
function Window:GetMinSize() end

---@param maxSize Vector
function Window:SetMaxSize(maxSize) end

---@return Vector
function Window:GetMaxSize() end

---@param size number
function Window:SetResizeHandleSize(size) end

---@return number
function Window:GetResizeHandleSize() end

---@param arg1 Widget
function Window:SetContentWidget(arg1) end

---@return Node
function Window:GetContentContainer() end

---@return Node
function Window:GetContentWidget() end

---@param color Vector
function Window:SetTitleBarColor(color) end

---@return Vector
function Window:GetTitleBarColor() end

---@param color Vector
function Window:SetBackgroundColor(color) end

---@return Vector
function Window:GetBackgroundColor() end

---@param texture? Texture
function Window:SetBackgroundTexture(texture) end

---@return Asset
function Window:GetBackgroundTexture() end

---@param texture? Texture
function Window:SetTitleBarTexture(texture) end

---@return Asset
function Window:GetTitleBarTexture() end

---@param texture? Texture
function Window:SetCloseButtonTexture(texture) end

---@return Asset
function Window:GetCloseButtonTexture() end

---@param color Vector
function Window:SetCloseButtonNormalColor(color) end

---@return Vector
function Window:GetCloseButtonNormalColor() end

---@param color Vector
function Window:SetCloseButtonHoveredColor(color) end

---@return Vector
function Window:GetCloseButtonHoveredColor() end

---@param color Vector
function Window:SetCloseButtonPressedColor(color) end

---@return Vector
function Window:GetCloseButtonPressedColor() end

---@return Node
function Window:GetTitleBar() end

---@return Node
function Window:GetTitleText() end

---@return Node
function Window:GetCloseButton() end

---@return Node
function Window:GetResizeHandle() end

---@return Node
function Window:GetBackground() end
