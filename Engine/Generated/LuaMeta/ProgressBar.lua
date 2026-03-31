--- @meta

---@class ProgressBar : Widget
ProgressBar = {}

---@param value number
function ProgressBar:SetValue(value) end

---@return number
function ProgressBar:GetValue() end

---@param value number
function ProgressBar:SetMinValue(value) end

---@return number
function ProgressBar:GetMinValue() end

---@param value number
function ProgressBar:SetMaxValue(value) end

---@return number
function ProgressBar:GetMaxValue() end

---@return number
function ProgressBar:GetRatio() end

---@param show boolean
function ProgressBar:SetShowPercentage(show) end

---@return boolean
function ProgressBar:IsShowingPercentage() end

---@param color Vector
function ProgressBar:SetBackgroundColor(color) end

---@return Vector
function ProgressBar:GetBackgroundColor() end

---@param color Vector
function ProgressBar:SetFillColor(color) end

---@return Vector
function ProgressBar:GetFillColor() end

---@param color Vector
function ProgressBar:SetTextColor(color) end

---@return Vector
function ProgressBar:GetTextColor() end

---@return Node
function ProgressBar:GetBackgroundQuad() end

---@return Node
function ProgressBar:GetFillQuad() end

---@return Node
function ProgressBar:GetTextWidget() end
