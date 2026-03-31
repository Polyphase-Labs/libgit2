--- @meta

---@class CheckBox : Widget
CheckBox = {}

---@param checked boolean
function CheckBox:SetChecked(checked) end

---@return boolean
function CheckBox:IsChecked() end

function CheckBox:Toggle() end

---@param text string
function CheckBox:SetText(text) end

---@return string
function CheckBox:GetText() end

---@param color Vector
function CheckBox:SetCheckedColor(color) end

---@return Vector
function CheckBox:GetCheckedColor() end

---@param color Vector
function CheckBox:SetUncheckedColor(color) end

---@return Vector
function CheckBox:GetUncheckedColor() end

---@param color Vector
function CheckBox:SetTextColor(color) end

---@return Vector
function CheckBox:GetTextColor() end

---@param size number
function CheckBox:SetBoxSize(size) end

---@return number
function CheckBox:GetBoxSize() end

---@param spacing number
function CheckBox:SetSpacing(spacing) end

---@return number
function CheckBox:GetSpacing() end

---@return Node
function CheckBox:GetBoxQuad() end

---@return Node
function CheckBox:GetTextWidget() end
