--- @meta

---@class ToolTipWidget : Widget
ToolTipWidget = {}

---@param value string
function ToolTipWidget:SetTooltipTitle(value) end

---@return string
function ToolTipWidget:GetTooltipTitle() end

---@param value string
function ToolTipWidget:SetTooltipText(value) end

---@return string
function ToolTipWidget:GetTooltipText() end

---@param name string
---@param desc string
function ToolTipWidget:SetContent(name, desc) end

---@param source Widget
function ToolTipWidget:ConfigureFromWidget(source) end

---@param color Vector
function ToolTipWidget:SetBackgroundColor(color) end

---@return Vector
function ToolTipWidget:GetBackgroundColor() end

---@param value number
function ToolTipWidget:SetCornerRadius(value) end

---@return number
function ToolTipWidget:GetCornerRadius() end

---@param value number
function ToolTipWidget:SetTitleFontSize(value) end

---@return number
function ToolTipWidget:GetTitleFontSize() end

---@param value number
function ToolTipWidget:SetTextFontSize(value) end

---@return number
function ToolTipWidget:GetTextFontSize() end

---@param color Vector
function ToolTipWidget:SetTitleColor(color) end

---@return Vector
function ToolTipWidget:GetTitleColor() end

---@param color Vector
function ToolTipWidget:SetTextColor(color) end

---@return Vector
function ToolTipWidget:GetTextColor() end

---@param left number
---@param top number
---@param right number
---@param bottom number
function ToolTipWidget:SetPadding(left, top, right, bottom) end

---@return number
function ToolTipWidget:GetPaddingLeft() end

---@return number
function ToolTipWidget:GetPaddingTop() end

---@return number
function ToolTipWidget:GetPaddingRight() end

---@return number
function ToolTipWidget:GetPaddingBottom() end

---@param value number
function ToolTipWidget:SetMaxWidth(value) end

---@return number
function ToolTipWidget:GetMaxWidth() end

---@param value number
function ToolTipWidget:SetTitleTextSpacing(value) end

---@return number
function ToolTipWidget:GetTitleTextSpacing() end
