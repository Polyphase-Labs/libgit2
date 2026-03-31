--- @meta

---@class SpinBox : Widget
SpinBox = {}

---@param value number
function SpinBox:SetValue(value) end

---@return number
function SpinBox:GetValue() end

---@param value number
function SpinBox:SetMinValue(value) end

---@return number
function SpinBox:GetMinValue() end

---@param value number
function SpinBox:SetMaxValue(value) end

---@return number
function SpinBox:GetMaxValue() end

---@param step number
function SpinBox:SetStep(step) end

---@return number
function SpinBox:GetStep() end

---@param prefix string
function SpinBox:SetPrefix(prefix) end

---@return string
function SpinBox:GetPrefix() end

---@param suffix string
function SpinBox:SetSuffix(suffix) end

---@return string
function SpinBox:GetSuffix() end

---@param color Vector
function SpinBox:SetBackgroundColor(color) end

---@return Vector
function SpinBox:GetBackgroundColor() end

---@param color Vector
function SpinBox:SetTextColor(color) end

---@return Vector
function SpinBox:GetTextColor() end

---@param color Vector
function SpinBox:SetButtonColor(color) end

---@return Vector
function SpinBox:GetButtonColor() end

---@return Node
function SpinBox:GetBackground() end

---@return Node
function SpinBox:GetTextWidget() end

---@return Node
function SpinBox:GetIncrementButton() end

---@return Node
function SpinBox:GetDecrementButton() end
