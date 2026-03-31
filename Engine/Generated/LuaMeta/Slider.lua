--- @meta

---@class Slider : Widget
Slider = {}

---@param value number
function Slider:SetValue(value) end

---@return number
function Slider:GetValue() end

---@param value number
function Slider:SetMinValue(value) end

---@return number
function Slider:GetMinValue() end

---@param value number
function Slider:SetMaxValue(value) end

---@return number
function Slider:GetMaxValue() end

---@param value number
function Slider:SetStep(value) end

---@return number
function Slider:GetStep() end

---@param minValue number
---@param maxValue number
function Slider:SetRange(minValue, maxValue) end

---@return boolean
function Slider:IsDragging() end

---@param value number
function Slider:SetGrabberWidth(value) end

---@return number
function Slider:GetGrabberWidth() end

---@param value number
function Slider:SetTrackHeight(value) end

---@return number
function Slider:GetTrackHeight() end

---@param color Vector
function Slider:SetBackgroundColor(color) end

---@return Vector
function Slider:GetBackgroundColor() end

---@param color Vector
function Slider:SetGrabberColor(color) end

---@return Vector
function Slider:GetGrabberColor() end

---@param color Vector
function Slider:SetGrabberHoveredColor(color) end

---@return Vector
function Slider:GetGrabberHoveredColor() end

---@param color Vector
function Slider:SetGrabberPressedColor(color) end

---@return Vector
function Slider:GetGrabberPressedColor() end

---@return Node
function Slider:GetBackground() end

---@return Node
function Slider:GetGrabber() end
