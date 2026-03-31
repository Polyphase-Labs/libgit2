--- @meta

---@class DialogWindow : Window
DialogWindow = {}

---@param text string
function DialogWindow:SetConfirmText(text) end

---@return string
function DialogWindow:GetConfirmText() end

---@param color Vector
function DialogWindow:SetConfirmNormalColor(color) end

---@return Vector
function DialogWindow:GetConfirmNormalColor() end

---@param color Vector
function DialogWindow:SetConfirmHoveredColor(color) end

---@return Vector
function DialogWindow:GetConfirmHoveredColor() end

---@param color Vector
function DialogWindow:SetConfirmPressedColor(color) end

---@return Vector
function DialogWindow:GetConfirmPressedColor() end

---@param texture? Texture
function DialogWindow:SetConfirmTexture(texture) end

---@return Asset
function DialogWindow:GetConfirmTexture() end

---@param show boolean
function DialogWindow:SetShowConfirmButton(show) end

---@return boolean
function DialogWindow:GetShowConfirmButton() end

---@param text string
function DialogWindow:SetRejectText(text) end

---@return string
function DialogWindow:GetRejectText() end

---@param color Vector
function DialogWindow:SetRejectNormalColor(color) end

---@return Vector
function DialogWindow:GetRejectNormalColor() end

---@param color Vector
function DialogWindow:SetRejectHoveredColor(color) end

---@return Vector
function DialogWindow:GetRejectHoveredColor() end

---@param color Vector
function DialogWindow:SetRejectPressedColor(color) end

---@return Vector
function DialogWindow:GetRejectPressedColor() end

---@param texture? Texture
function DialogWindow:SetRejectTexture(texture) end

---@return Asset
function DialogWindow:GetRejectTexture() end

---@param show boolean
function DialogWindow:SetShowRejectButton(show) end

---@return boolean
function DialogWindow:GetShowRejectButton() end

---@param height number
function DialogWindow:SetButtonBarHeight(height) end

---@return number
function DialogWindow:GetButtonBarHeight() end

---@param spacing number
function DialogWindow:SetButtonSpacing(spacing) end

---@return number
function DialogWindow:GetButtonSpacing() end

---@param alignment integer
function DialogWindow:SetButtonBarAlignment(alignment) end

---@return integer
function DialogWindow:GetButtonBarAlignment() end

---@param color Vector
function DialogWindow:SetButtonBarColor(color) end

---@return Vector
function DialogWindow:GetButtonBarColor() end

---@param width number
function DialogWindow:SetButtonWidth(width) end

---@return number
function DialogWindow:GetButtonWidth() end

---@param padding number
function DialogWindow:SetButtonBarPadding(padding) end

---@return number
function DialogWindow:GetButtonBarPadding() end

function DialogWindow:Confirm() end

function DialogWindow:Reject() end

---@return Node
function DialogWindow:GetButtonBar() end

---@return Node
function DialogWindow:GetConfirmButton() end

---@return Node
function DialogWindow:GetRejectButton() end
