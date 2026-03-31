--- @meta

---@class ScrollContainer : Widget
ScrollContainer = {}

---@param x number
---@param y number
function ScrollContainer:SetScrollOffset(x, y) end

---@return number, number
function ScrollContainer:GetScrollOffset() end

---@param x number
function ScrollContainer:SetScrollOffsetX(x) end

---@param y number
function ScrollContainer:SetScrollOffsetY(y) end

function ScrollContainer:ScrollToTop() end

function ScrollContainer:ScrollToBottom() end

function ScrollContainer:ScrollToLeft() end

function ScrollContainer:ScrollToRight() end

---@param mode integer
function ScrollContainer:SetScrollSizeMode(mode) end

---@return integer
function ScrollContainer:GetScrollSizeMode() end

---@param mode integer
function ScrollContainer:SetHorizontalScrollbarMode(mode) end

---@return integer
function ScrollContainer:GetHorizontalScrollbarMode() end

---@param mode integer
function ScrollContainer:SetVerticalScrollbarMode(mode) end

---@return integer
function ScrollContainer:GetVerticalScrollbarMode() end

---@param width number
function ScrollContainer:SetScrollbarWidth(width) end

---@return number
function ScrollContainer:GetScrollbarWidth() end

---@param speed number
function ScrollContainer:SetScrollSpeed(speed) end

---@return number
function ScrollContainer:GetScrollSpeed() end

---@param enabled boolean
function ScrollContainer:SetMomentumEnabled(enabled) end

---@return boolean
function ScrollContainer:IsMomentumEnabled() end

---@param friction number
function ScrollContainer:SetMomentumFriction(friction) end

---@return number
function ScrollContainer:GetMomentumFriction() end

---@return boolean
function ScrollContainer:CanScrollHorizontally() end

---@return boolean
function ScrollContainer:CanScrollVertically() end

---@return number, number
function ScrollContainer:GetContentSize() end

---@return number, number
function ScrollContainer:GetMaxScrollOffset() end

---@return boolean
function ScrollContainer:IsDragging() end

---@return boolean
function ScrollContainer:IsScrolling() end

---@return Node
function ScrollContainer:GetContentWidget() end

---@param priority boolean
function ScrollContainer:SetChildInputPriority(priority) end

---@return boolean
function ScrollContainer:GetChildInputPriority() end

---@param color Vector
function ScrollContainer:SetScrollbarColor(color) end

---@return Vector
function ScrollContainer:GetScrollbarColor() end

---@param color Vector
function ScrollContainer:SetScrollbarHoveredColor(color) end

---@return Vector
function ScrollContainer:GetScrollbarHoveredColor() end

---@param color Vector
function ScrollContainer:SetScrollbarTrackColor(color) end

---@return Vector
function ScrollContainer:GetScrollbarTrackColor() end

---@param texture? Texture
function ScrollContainer:SetScrollbarTexture(texture) end

---@return Asset
function ScrollContainer:GetScrollbarTexture() end

---@param texture? Texture
function ScrollContainer:SetTrackTexture(texture) end

---@return Asset
function ScrollContainer:GetTrackTexture() end

---@param show boolean
function ScrollContainer:SetShowScrollButtons(show) end

---@return boolean
function ScrollContainer:GetShowScrollButtons() end

---@param size number
function ScrollContainer:SetButtonSize(size) end

---@return number
function ScrollContainer:GetButtonSize() end

---@param texture? Texture
function ScrollContainer:SetUpButtonTexture(texture) end

---@return Asset
function ScrollContainer:GetUpButtonTexture() end

---@param texture? Texture
function ScrollContainer:SetDownButtonTexture(texture) end

---@return Asset
function ScrollContainer:GetDownButtonTexture() end

---@param texture? Texture
function ScrollContainer:SetLeftButtonTexture(texture) end

---@return Asset
function ScrollContainer:GetLeftButtonTexture() end

---@param texture? Texture
function ScrollContainer:SetRightButtonTexture(texture) end

---@return Asset
function ScrollContainer:GetRightButtonTexture() end

---@param color Vector
function ScrollContainer:SetButtonColor(color) end

---@return Vector
function ScrollContainer:GetButtonColor() end

---@return Node
function ScrollContainer:GetHScrollbar() end

---@return Node
function ScrollContainer:GetVScrollbar() end

---@return Node
function ScrollContainer:GetHTrack() end

---@return Node
function ScrollContainer:GetVTrack() end

---@return Node
function ScrollContainer:GetUpButton() end

---@return Node
function ScrollContainer:GetDownButton() end

---@return Node
function ScrollContainer:GetLeftButton() end

---@return Node
function ScrollContainer:GetRightButton() end
